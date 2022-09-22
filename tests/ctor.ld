ENTRY(entry)
LOAD_VIR = 0x100000;
LOAD_PHYS = 0x100000;
SECTIONS
{
  .text LOAD_VIR : AT(LOAD_PHYS)
  {
    _linker_code = .;
    *(.text)
    *(.text.*)
    *(.rodata*)
    *(.gnu.linkonce.t*)
    *(.gnu.linkonce.r*)
    . = ALIGN(4096);
  }
  .data :
  {
    _linker_data_start = .;
    __CTOR_LIST__ = .; LONG((__CTOR_END__ - __CTOR_LIST__) / 4 - 2)*
                                          (.ctors)   LONG(0) __CTOR_END__ = .;
          __DTOR_LIST__ = .; LONG((__DTOR_END__ - __DTOR_LIST__) / 4 - 2)*
                                         (.dtors) LONG(0) __DTOR_END__ = .;
    _linker_data = .;
    *(.data)
    *(.data.*)
    *(.gnu.linkonce.d*)
    . = ALIGN(4096);
  }
  .bss :
  {
    _linker_bss = .;
    *(.bss)
    *(.bss.*)
    *(.gnu.linkonce.b.*)
    *(.COMMON)
    . = ALIGN(4096);
  }
  _linker_end = .;
}
