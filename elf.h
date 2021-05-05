#pragma once

#define Elf64_Addr uint64_t
#define Elf64_Half uint16_t
#define Elf64_Off uint64_t
#define Elf64_Sword int32_t
#define Elf64_Word uint32_t
#define Elf64_Xword uint64_t
#define Elf64_Sxord int64_t

#define EI_NIDENT 16
typedef struct {
  unsigned char e_ident[EI_NIDENT];
  Elf64_Half e_type;
  Elf64_Half e_machine;
  Elf64_Word e_version;
  Elf64_Addr e_entry;
  Elf64_Off e_phoff;
  Elf64_Off e_shoff;
  Elf64_Word e_flags;
  Elf64_Half e_ehsize;
  Elf64_Half e_phentsize;
  Elf64_Half e_phnum;
  Elf64_Half e_shentsize;
  Elf64_Half e_shnum;
  Elf64_Half e_shstrndx;
} Elf64_Ehdr;

#define ELFCLASS64 2
typedef struct {
  Elf64_Word sh_name;
  Elf64_Word sh_type;
  Elf64_Word sh_flags;
  Elf64_Addr sh_addr;
  Elf64_Off sh_offset;
  Elf64_Word sh_size;
  Elf64_Word sh_link;
  Elf64_Word sh_info;
  Elf64_Word sh_addralign;
  Elf64_Word sh_entsize;
} Elf64_Shdr;

typedef struct {
  Elf64_Word p_type;
  Elf64_Word p_flags;
  Elf64_Off p_offset;
  Elf64_Addr p_vaddr;
  Elf64_Addr p_paddr;
  Elf64_Xword p_filesz;
  Elf64_Xword p_memsz;
  Elf64_Xword p_align;
} Elf64_Phdr;

#define PT_LOAD 1
