#include <am.h>
#include <klib-macros.h>

// timer.c
void __am_timer_init();
void __am_timer_rtc(AM_TIMER_RTC_T *);
void __am_timer_uptime(AM_TIMER_UPTIME_T *);

// input.c
void __am_input_keybrd(AM_INPUT_KEYBRD_T *);

// 音频
void __am_audio_config(AM_AUDIO_CONFIG_T *cfg) { cfg->present = false; }
void __am_audio_ctrl(AM_AUDIO_CTRL_T *ctl) {}
void __am_audio_status(AM_AUDIO_STATUS_T *stat) { stat->count = 0; }
void __am_audio_play(AM_AUDIO_PLAY_T *play) {}

// GPU (VGA 帧缓冲, 640x480, 每像素 4 字节, 映射到 0x2100_0000)
#define VGA_BASE   0x21000000L
#define VGA_WIDTH  640
#define VGA_HEIGHT 480

void __am_gpu_config(AM_GPU_CONFIG_T *cfg) {
    cfg->present   = true;
    cfg->has_accel = false;
    cfg->width     = VGA_WIDTH;
    cfg->height    = VGA_HEIGHT;
    cfg->vmemsz    = VGA_WIDTH * VGA_HEIGHT * 4;
}
void __am_gpu_status(AM_GPU_STATUS_T *status) { status->ready = true; }
void __am_gpu_fbdraw(AM_GPU_FBDRAW_T *ctl) {
    uint32_t *fb  = (uint32_t *)(VGA_BASE + (ctl->y * VGA_WIDTH + ctl->x) * 4);
    uint32_t *pix = (uint32_t *)ctl->pixels;
    if (pix != NULL) {
        for (int j = 0; j < ctl->h; j++)
            for (int i = 0; i < ctl->w; i++)
                fb[j * VGA_WIDTH + i] = pix[j * ctl->w + i];
    }
    // NVBoard 的 VGA 自动刷新, 无需处理 sync
    (void)ctl->sync;
}

// 输入设备 (__am_input_keybrd 在 input.c 中实现: 读取 PS/2 扫描码并翻译)
void __am_input_config(AM_INPUT_CONFIG_T *cfg) { cfg->present = true; }

// 其他设备
#define UART_BASE 0x10000000L
void __am_uart_config(AM_UART_CONFIG_T *cfg) { cfg->present = true; }
void __am_uart_tx(AM_UART_TX_T *tx) {
    volatile uint8_t *u = (volatile uint8_t *)UART_BASE;
    while ((u[5] & 0x20) == 0) ;    // 等 THRE
    u[0] = (uint8_t)tx->data;
}
void __am_uart_rx(AM_UART_RX_T *rx) {
    volatile uint8_t *u = (volatile uint8_t *)UART_BASE;
    if (u[5] & 0x01) rx->data = (char)u[0];   // LSR.DR: 有数据可读
    else             rx->data = (char)0xff;
}
void __am_disk_config(AM_DISK_CONFIG_T *cfg) { cfg->present = false; }
void __am_disk_status(AM_DISK_STATUS_T *stat) { stat->ready = false; }
void __am_disk_blkio(AM_DISK_BLKIO_T *io) {}
void __am_net_config(AM_NET_CONFIG_T *cfg) { cfg->present = false; }

// 定时器
static void __am_timer_config(AM_TIMER_CONFIG_T *cfg) {
    cfg->present = true;
    cfg->has_rtc = true;
}

// 查找表
typedef void (*handler_t)(void *buf);
static void *lut[128] = {
    [AM_TIMER_CONFIG] = __am_timer_config,
    [AM_TIMER_RTC   ] = __am_timer_rtc,
    [AM_TIMER_UPTIME] = __am_timer_uptime,
    [AM_INPUT_CONFIG] = __am_input_config,
    [AM_INPUT_KEYBRD] = __am_input_keybrd,
    [AM_UART_CONFIG ] = __am_uart_config,
    [AM_UART_TX     ] = __am_uart_tx,
    [AM_UART_RX     ] = __am_uart_rx,
    [AM_AUDIO_CONFIG] = __am_audio_config,
    [AM_AUDIO_CTRL  ] = __am_audio_ctrl,
    [AM_AUDIO_STATUS] = __am_audio_status,
    [AM_AUDIO_PLAY  ] = __am_audio_play,
    [AM_GPU_CONFIG  ] = __am_gpu_config,
    [AM_GPU_STATUS  ] = __am_gpu_status,
    [AM_GPU_FBDRAW  ] = __am_gpu_fbdraw,
    [AM_DISK_CONFIG ] = __am_disk_config,
    [AM_DISK_STATUS ] = __am_disk_status,
    [AM_DISK_BLKIO  ] = __am_disk_blkio,
    [AM_NET_CONFIG  ] = __am_net_config,
};

static void fail(void *buf) { panic("access nonexist register"); }

bool ioe_init() {
    for (int i = 0; i < LENGTH(lut); i++)
        if (!lut[i]) lut[i] = fail;
    __am_timer_init();
    return true;
}

void ioe_read(int reg, void *buf) { ((handler_t)lut[reg])(buf); }
void ioe_write(int reg, void *buf) { ((handler_t)lut[reg])(buf); }