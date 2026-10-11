ENTRY(_start)

MEMORY {
  mrom (rx)  : ORIGIN = 0x20000000, LENGTH = 4K
  sram (rwx) : ORIGIN = 0x0f000000, LENGTH = 8K
}

SECTIONS {
  . = ORIGIN(mrom);
  .text : {
    *(entry)
    *(.text*)
  } > mrom
  etext = .;
  _etext = .;

  .rodata : {
    *(.rodata*)
  } > mrom

  /* .data: 运行地址 (VMA) 在 SRAM, 加载地址 (LMA) 紧跟 .rodata 在 MROM */
  . = ORIGIN(sram);
  .data : {
    _data_start = .;
    *(.data*)
    *(.sdata*)
    *(.srodata*)
    _data_end = .;
  } > sram AT> mrom
  _data_lma_start = LOADADDR(.data);

  .bss : {
    _bss_start = .;
    *(.bss*)
    *(.sbss*)
    *(.scommon)
    _bss_end = .;
  } > sram

  . = ALIGN(0x10);
  _heap_start = .;

  _stack_top = ORIGIN(sram) + LENGTH(sram);
  _stack_pointer = _stack_top;
  end = .;
  _end = .;
}
