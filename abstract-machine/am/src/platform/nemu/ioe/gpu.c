#include <am.h>
#include <nemu.h>
#include <klib.h>

#define SYNC_ADDR (VGACTL_ADDR + 4)

static int screen_w = 0;
static int screen_h = 0;

void __am_gpu_init() {
  uint32_t vgactl = inl(VGACTL_ADDR);
  screen_w = (vgactl >> 16) & 0xffff;   // 高16位：宽度
  screen_h = vgactl & 0xffff;           // 低16位：高度

  /* //测试代码：填充红色并同步
    uint32_t *fb = (uint32_t *)(uintptr_t)FB_ADDR;
    for (int i = 0; i < screen_w * screen_h; i++) {
        fb[i] = 0xff0000;
    }
    outl(SYNC_ADDR, 1);
  */
}

void __am_gpu_config(AM_GPU_CONFIG_T *cfg) {
  *cfg = (AM_GPU_CONFIG_T) {
    .present = true, 
    .has_accel = false,
    .width = screen_w, 
    .height = screen_h,
    .vmemsz = screen_w * screen_h * 4
  };
}

void __am_gpu_fbdraw(AM_GPU_FBDRAW_T *ctl) {
  if (ctl->pixels == NULL) return;
  int x = ctl->x, y = ctl->y;
  int w = ctl->w, h = ctl->h;
  uint32_t *fb = (uint32_t *)FB_ADDR;
  uint32_t *pixels = ctl->pixels;

  for (int j = 0; j < h; j++) {
    for (int i = 0; i < w; i++) {
      fb[(y + j) * screen_w + (x + i)] = pixels[j * w + i];
    }
  }

  
  outl(SYNC_ADDR, 1);

}

void __am_gpu_status(AM_GPU_STATUS_T *status) {
  status->ready = true;
}