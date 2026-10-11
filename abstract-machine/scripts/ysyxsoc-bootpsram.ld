ENTRY(_start)

MEMORY {
  flash (rx)  : ORIGIN = 0x30000000, LENGTH = 16M
  sram  (rwx) : ORIGIN = 0x0f000000, LENGTH = 8K
  psram (rwx) : ORIGIN = 0x80000000, LENGTH = 4M
}

SECTIONS {
  /* FSBL: 在 flash 中执行 (XIP) */
  . = ORIGIN(flash);
  .boot : { KEEP(*(entry)) } > flash

  /* 拷贝表 (flash, 每项 lma/vma/size, size==0 结束) */
  .copytab : {
    _copy_table = .;
    LONG(LOADADDR(.text));       LONG(ADDR(.text));       LONG(SIZEOF(.text));
    LONG(LOADADDR(.rodata));     LONG(ADDR(.rodata));     LONG(SIZEOF(.rodata));
    LONG(LOADADDR(.data.extra)); LONG(ADDR(.data.extra)); LONG(SIZEOF(.data.extra));
    LONG(LOADADDR(.data));       LONG(ADDR(.data));       LONG(SIZEOF(.data));
    LONG(0); LONG(0); LONG(0);
  } > flash

  /* SSBL: VMA 在 SRAM, LMA 在 flash (FSBL 搬运) */
  .ssbl : { KEEP(*(ssbl)) } > sram AT> flash

  /* 程序代码/只读数据: VMA 在 PSRAM, LMA 在 flash */
  . = ORIGIN(psram);
  .text : { *(.text*) } > psram AT> flash
  .rodata : { *(.rodata*) } > psram AT> flash

  .data : {
    _data_start = .;
    *(.data*)
    *(.sdata*)
    *(.srodata*)
    _data_end = .;
  } > psram AT> flash
  _data_lma_start = LOADADDR(.data);

  /* RT-Thread 的 .data.extra (extra.ld 定义) 也要 LMA 在 flash */
  .data.extra : {
    __fsymtab_start = .; KEEP(*(FSymTab)); __fsymtab_end = .;
    . = ALIGN(8);
    __vsymtab_start = .; KEEP(*(VSymTab)); __vsymtab_end = .;
    . = ALIGN(8);
    __rt_init_start = .; KEEP(*(SORT(.rti_fn*))); __rt_init_end = .;
    . = ALIGN(8);
    __rt_utest_tc_tab_start = .; KEEP(*(UtestTcTab)); __rt_utest_tc_tab_end = .;
    . = ALIGN(8);
    __am_apps_data_start = .; *(__am_apps.data*) *(__am_apps.sdata*); __am_apps_data_end = .;
    . = ALIGN(8);
  } > psram AT> flash

  .bss : {
    _bss_start = .;
    *(.bss*)
    *(.sbss*)
    *(.scommon)
    _bss_end = .;
  } > psram

  .bss.extra : {
    . = ALIGN(8);
    __am_apps_bss_start = .; *(__am_apps.bss*) *(__am_apps.sbss*) *(__am_apps.scommon*); __am_apps_bss_end = .;
    . = ALIGN(8);
  } > psram

  . = ALIGN(0x10);
  _heap_start = .;

  _stack_top = ORIGIN(psram) + LENGTH(psram);
  _stack_pointer = _stack_top;
  end = .;
  _end = .;

  /* bootloader 搬运范围 */
  _ssbl_lma      = LOADADDR(.ssbl);
  _ssbl_lma_end  = LOADADDR(.ssbl) + SIZEOF(.ssbl);
  _ssbl_start    = ADDR(.ssbl);
  _load_start = LOADADDR(.text);
  _load_end   = LOADADDR(.data) + SIZEOF(.data);
  _run_start  = ADDR(.text);
}
