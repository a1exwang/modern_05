ENTRY(init)
OUTPUT_FORMAT(elf64-x86-64)
SECTIONS
{
/* Currently using hack to let linker script file the object file */
  .text.start (0x200000) : {
    CMakeFiles/kernel.dir/init/init.cpp.o( .text )
  }

  .text (0x201000) : ALIGN(0x1000) {
_TEXT_START_ = .;
    *(.text)
_TEXT_END_ = .;
  }

  .data (0x300000): ALIGN(0x1000) {
_DATA_START_ = .;
    *(.data)
_DATA_END_ = .;
  }

  .bss (0x1000000): ALIGN(0x1000) {
_BSS_START_ = .;
    *(.bss)
_BSS_END_ = .;
  }
}
