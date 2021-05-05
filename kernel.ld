ENTRY(init)
OUTPUT_FORMAT(elf64-x86-64)
SECTIONS
{
/* Currently using hack to let linker script file the object file */
  .text.start (0xffff800000100000) : {
    CMakeFiles/kernel.dir/init/init.cpp.o( .text )
  }

  .text (0xffff800000110000) : ALIGN(0x1000) {
_TEXT_START_ = .;
    *(.text)
_TEXT_END_ = .;
  }

  .data (0xffff800000300000): ALIGN(0x1000) {
_DATA_START_ = .;
    *(.data)
_DATA_END_ = .;
  }

  .bss (0xffff800000500000): ALIGN(0x1000) {
_BSS_START_ = .;
    *(.bss)
_BSS_END_ = .;
  }

  /* put these useless sections at the end of 32MiB */
  .note.gnu.build-id (0xffff800001f00000): ALIGN(0x1000) {
    *(.note.gnu.build-id)
  }
}
