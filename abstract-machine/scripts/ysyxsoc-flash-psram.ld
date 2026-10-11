ENTRY(_start)

MEMORY {
  flash (rx)  : ORIGIN = 0x30000000, LENGTH = 16M
  psram (rwx) : ORIGIN = 0x80000000, LENGTH = 4M
}

SECTIONS {
  . = ORIGIN(flash);
  .text : {
    *(entry)
    *(.text*)
  } > flash
  etext = .;
  _etext = .;

  .rodata : {
    *(.rodata*)
  } > flash

  /* .data: VMA 在 PSRAM, LMA 紧跟 .rodata 在 flash (bootloader 搬运) */
  . = ORIGIN(psram);
  .data : {
    _data_start = .;
    *(.data*)
    *(.sdata*)
    *(.srodata*)
    _data_end = .;
  } > psram AT> flash
  _data_lma_start = LOADADDR(.data);

  .bss : {
    _bss_start = .;
    *(.bss*)
    *(.sbss*)
    *(.scommon)
    _bss_end = .;
  } > psram

  . = ALIGN(0x10);
  _heap_start = .;

  _stack_top = ORIGIN(psram) + LENGTH(psram);
  _stack_pointer = _stack_top;
  end = .;
  _end = .;
}
