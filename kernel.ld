ENTRY(init)
OUTPUT_FORMAT(elf64-x86-64)
SECTIONS
{
/* Currently using hack to let linker script file the object file */
  .text.start (0xffff800000100000) : {
    CMakeFiles/kernellib.dir/init/init.cpp.o( .text )
  }

  .text (0xffff800000110000) : ALIGN(0x1000) {
_TEXT_START_ = .;
    *(.text.unlikely .text.*_unlikely .text.unlikely.*)
    *(.text.exit .text.exit.*)
    *(.text.startup .text.startup.*)
    *(.text.hot .text.hot.*)
    *(.text .stub .text.* .gnu.linkonce.t.*)
    /* .gnu.warning sections are handled specially by elf32.em.  */
    *(.gnu.warning)
_TEXT_END_ = .;
  }

  .rodata : ALIGN(0x1000) { *(.rodata .rodata.* .gnu.linkonce.r.*) }

  .data.rel.ro : ALIGN(0x1000) { *(.data.rel.ro.local* .gnu.linkonce.d.rel.ro.local.*) *(.data.rel.ro .data.rel.ro.* .gnu.linkonce.d.rel.ro.*) }

  .eh_frame_hdr   : ALIGN(0x1000) { *(.eh_frame_hdr) *(.eh_frame_entry .eh_frame_entry.*) }
  .eh_frame       : ALIGN(0x1000) ONLY_IF_RO { KEEP (*(.eh_frame)) *(.eh_frame.*) }
  .eh_frame       : ALIGN(0x1000) ONLY_IF_RW { KEEP (*(.eh_frame)) *(.eh_frame.*) }

  .got            : ALIGN(0x1000) { *(.got) *(.igot) }
  .got.plt        : ALIGN(0x1000) { *(.got.plt) *(.igot.plt) }

  .data (0xffff800000200000): ALIGN(0x1000) {
_DATA_START_ = .;
  *(.data .data.* .gnu.linkonce.d.*)
_DATA_END_ = .;
  }

  .bss (0xffff800001000000): ALIGN(0x1000) {
_BSS_START_ = .;
    *(.bss)
_BSS_END_ = .;
  }

  .data.per_cpu (0xffff800001e00000): ALIGN(0x1000) {
    *(.data.per_cpu)
  }

  /* put these useless sections at the end */
  .note.gnu.build-id (0xffff800001f00000): ALIGN(0x1000) {
    *(.note.gnu.build-id)
  }

  .init_array (0xffff800001f10000): ALIGN(0x1000) {
_INIT_ARRAY_START_ = .;
    *(.init_array)
_INIT_ARRAY_END_ = .;
  }
}
