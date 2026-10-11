ENTRY(_start)
MEMORY {
  flash (rx)  : ORIGIN = 0x30000000, LENGTH = 16M
  sram  (rwx) : ORIGIN = 0x0f000000, LENGTH = 8K
}
SECTIONS {
  . = ORIGIN(flash);
  .boot : { KEEP(*(entry)) } > flash

  /* 拷贝表 (flash, 每项 lma/vma/size; lma==0 结束) */
  .copytab : {
    _copy_table = .;
    LONG(LOADADDR(.text));   LONG(ADDR(.text));   LONG(SIZEOF(.text));
    LONG(LOADADDR(.rodata)); LONG(ADDR(.rodata)); LONG(SIZEOF(.rodata));
    LONG(LOADADDR(.data));   LONG(ADDR(.data));   LONG(SIZEOF(.data));
    LONG(0); LONG(0); LONG(0);
  } > flash

  /* SSBL: VMA 在 SRAM, LMA 在 flash */
  .ssbl : { KEEP(*(ssbl)) } > sram AT> flash

  /* 程序: VMA 在 SRAM (紧跟 SSBL), LMA 在 flash */
  . = ORIGIN(sram) + SIZEOF(.ssbl);
  .text : { *(.text*) } > sram AT> flash
  .rodata : { *(.rodata*) } > sram AT> flash
  .data : {
    _data_start = .;
    *(.data*) *(.sdata*) *(.srodata*)
    _data_end = .;
  } > sram AT> flash
  _data_lma_start = LOADADDR(.data);
  .bss : {
    _bss_start = .;
    *(.bss*) *(.sbss*) *(.scommon)
    _bss_end = .;
  } > sram

  . = ALIGN(0x10);
  _heap_start = .;
  _stack_top = ORIGIN(sram) + LENGTH(sram);
  _stack_pointer = _stack_top;
  end = .;
  _end = .;

  _ssbl_lma     = LOADADDR(.ssbl);
  _ssbl_lma_end = LOADADDR(.ssbl) + SIZEOF(.ssbl);
  _ssbl_start   = ADDR(.ssbl);
}
