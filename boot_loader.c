#include <efi.h>
#include <efilib.h>
#include <efidef.h>
#include <stdarg.h>

#include "efiConsoleControl.h"
#include "efiUgaDraw.h"
#include "include/init/efi_info.h"
#include "include/cpu_defs.h"

unsigned short port = 0x3f8;

void outb(u16 address, u8 data) {
  asm (
  "mov %1, %%al\n"
  "mov %0, %%dx\n"
  "outb %%al, %%dx\n"
  :
  : "r"(address), "r"(data)
  : "%dx", "%al"
  );
}
u8 inb(u16 address) {
  u8 result = 0;
  asm (
  "mov %1, %%dx\n"
  "inb %%dx, %%al\n"
  "mov %%al, %0\n"
  : "=r"(result)
  : "r"(address)
  : "%dx", "%al"
  );
  return result;
}

int is_transmit_empty() {
  return inb(port + 5) & 0x20;
}

void init_serial() {
  outb(port + 1, 0x00);    // Disable all interrupts
  outb(port + 3, 0x80);    // Enable DLAB (set baud rate divisor)
  outb(port + 0, 0x01);    // Set divisor to 3 (lo byte) 38400 baud
  outb(port + 1, 0x00);    //                  (hi byte)
  outb(port + 3, 0x03);    // 8 bits, no parity, one stop bit
  outb(port + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
  outb(port + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}
void putb(u8 byte) {
  while (is_transmit_empty() == 0);
  outb(port, byte);
}

void puts(const char *s) {
  while (*s != 0) {
    putb(*s);
    s++;
  }
}

void puti(u64 n) {
  char temp[32];
  const char *alphabet = "0123456789abcdef";

  u64 remain = n;
  u64 radix = 16;
  if (n == 0) {
    putb('0');
  } else {
    int i = 0;
    while (remain) {
      u64 newRemain = remain / radix;
      u64 digit = alphabet[remain % radix];
      temp[i] = digit;
      i++;

      remain = newRemain;
    }
    while (i > 0) {
      i--;
      putb(temp[i]);
    }
  }
}

/**


 rEFIt License
===============

Copyright (c) 2006 Christoph Pfisterer
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

 * Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the
   distribution.

 * Neither the name of Christoph Pfisterer nor the names of the
   contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

static EFI_GUID ConsoleControlProtocolGuid = EFI_CONSOLE_CONTROL_PROTOCOL_GUID;
static EFI_CONSOLE_CONTROL_PROTOCOL *ConsoleControl = NULL;

static EFI_GUID UgaDrawProtocolGuid = EFI_UGA_DRAW_PROTOCOL_GUID;
static EFI_UGA_DRAW_PROTOCOL *UgaDraw = NULL;

static EFI_GUID GraphicsOutputProtocolGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
static EFI_GRAPHICS_OUTPUT_PROTOCOL *GraphicsOutput = NULL;

static EFI_GUID LoadFileProtocolGuid = EFI_LOAD_FILE_PROTOCOL_GUID;
static EFI_LOAD_FILE_PROTOCOL *LoadFile = NULL;

static EFI_GUID SimpleFileSystemProtocolGuid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
static EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *SimpleFileSystem = NULL;

static EFI_GUID SimpleTextOutProtocolGuid = EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL_GUID;
static EFI_SIMPLE_TEXT_OUT_PROTOCOL *TextOut = NULL;

static EFI_GET_MEMORY_MAP EfiGetMemoryMap;

static BOOLEAN egHasGraphics = FALSE;
static UINTN egScreenWidth  = 800;
static UINTN egScreenHeight = 600;
// Make the necessary system calls to identify the current graphics mode.
// Stores the results in the file-global variables egScreenWidth,
// egScreenHeight, and egHasGraphics. The first two of these will be
// unchanged if neither GraphicsOutput nor UgaDraw is a valid pointer.
static VOID egDetermineScreenSize(VOID) {
  EFI_STATUS Status = EFI_SUCCESS;
  UINT32 UGAWidth, UGAHeight, UGADepth, UGARefreshRate;

  // get screen size
  egHasGraphics = FALSE;
  if (GraphicsOutput != NULL) {
    egScreenWidth = GraphicsOutput->Mode->Info->HorizontalResolution;
    egScreenHeight = GraphicsOutput->Mode->Info->VerticalResolution;
    egHasGraphics = TRUE;
  } else if (UgaDraw != NULL) {
    Status = UgaDraw->GetMode(UgaDraw, &UGAWidth, &UGAHeight, &UGADepth, &UGARefreshRate);
    if (EFI_ERROR(Status)) {
      UgaDraw = NULL;   // graphics not available
    } else {
      egScreenWidth  = UGAWidth;
      egScreenHeight = UGAHeight;
      egHasGraphics = TRUE;
    }
  }
} // static VOID egDetermineScreenSize()

VOID egInitScreen(VOID)
{
  Print(L"egInitScreen\n");
  EFI_STATUS Status = EFI_SUCCESS;

  // get protocols
  Status = LibLocateProtocol(&ConsoleControlProtocolGuid, (VOID **) &ConsoleControl);
  if (EFI_ERROR(Status))
    ConsoleControl = NULL;

  Status = LibLocateProtocol(&UgaDrawProtocolGuid, (VOID **) &UgaDraw);
  if (EFI_ERROR(Status))
    UgaDraw = NULL;

  Status = LibLocateProtocol(&GraphicsOutputProtocolGuid, (VOID **) &GraphicsOutput);
  if (EFI_ERROR(Status))
    GraphicsOutput = NULL;

  egDetermineScreenSize();
}

VOID egClearScreen()
{
  EFI_UGA_PIXEL FillColor;
  FillColor.Red   = 0xff;
  FillColor.Green = 0x0;
  FillColor.Blue  = 0x0;
  FillColor.Reserved = 0;

  if (GraphicsOutput != NULL) {
    // EFI_GRAPHICS_OUTPUT_BLT_PIXEL and EFI_UGA_PIXEL have the same
    // layout, and the header from TianoCore actually defines them
    // to be the same type.
    efi_call10(GraphicsOutput->Blt, (uint64_t)GraphicsOutput,(uint64_t)(EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)&FillColor, EfiBltVideoFill,
                        0, 0, 0, 0, egScreenWidth, egScreenHeight, 0);
    Print(L"using GraphicsOutput\n");
  } else if (UgaDraw != NULL) {
    efi_call10(UgaDraw->Blt, (uint64_t)UgaDraw, (uint64_t)&FillColor, EfiUgaVideoFill, 0, 0, 0, 0, egScreenWidth, egScreenHeight, 0);
    Print(L"using Uga\n");
  }

}

uint64_t get_cs() {
  uint64_t result;
  asm ("mov $0, %%rax;"
       "mov %%cs, %0"
  : "=r"(result)
  :
  : "%rax"
  );
  return result;
}

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

void efi_assert(int expr, const CHAR16 *s, ...) {
  va_list vl;
  va_start(vl, s);
  if (expr == 0) {
    VPrint(s, vl);
  }
  va_end(vl);
}

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

void copy(char *target, char *src, uint64_t size) {
  for (uint64_t i = 0; i < size; i++) {
    target[i] = src[i];
  }
}


struct EFIServicesInfo efi_info;

// buffer contains the elf file
typedef void (*EntrypointFunc)();
// phyKernelStart: physicalAddress(= virtAddr because EFI has identity mapping) where kernel will be loaded,
//  but later we will map this physicalAddress to KERNERL_START(0xffff8000_00000000)
EntrypointFunc LoadKernel(EFI_SYSTEM_TABLE* SystemTable, char *phyKernelStart, uint64_t kernel_max_size, char *buffer, uint64_t buffer_size) {
  Elf64_Ehdr *ehdr = buffer;
  const char *ElfMagic = "\x7f" "ELF";
  for (int i = 0; i < 4; i++) {
    if (ElfMagic[i] != buffer[i]) {
      Print(L"Corrupted ELF file, invalid header magic %x, %x, i = %d\n", (uint32_t)ElfMagic[i], (uint32_t)buffer[i], i);
      return 0;
    }
  }

  efi_assert(ehdr->e_phoff, L"program header table not found\n");
  Print(L"Kernel has %d EFI sections at 0x%08x\n", ehdr->e_phnum, ehdr->e_phoff);
  Print(L"  p_offset p_filesz p_vaddr p_memsz\n", ehdr->e_phnum, ehdr->e_phoff);
  u64 newMappings = 0;
  for (int i = 0; i < ehdr->e_phnum; i++) {
    Elf64_Phdr *phdr = (buffer + ehdr->e_phoff + ehdr->e_phentsize * i);
    if (phdr->p_type == PT_LOAD && phdr->p_memsz > 0) {
      Print(L"  0x%0lx 0x%0lx 0x%0lx 0x%0lx", phdr->p_offset, phdr->p_filesz, phdr->p_vaddr, phdr->p_memsz);
      if (phdr->p_filesz <= phdr->p_memsz) {
        UINT64 section_start = phdr->p_vaddr;
        UINT64 section_offset = section_start - KERNEL_START;
        if (section_offset + phdr->p_memsz > kernel_max_size) {
          Print(L"kernel file size too big to fill in the allocated buffer\n");
          return 0;
        }
        char *target_phy = phyKernelStart + section_offset;
        Print(L" map 0x%0lx -> 0x%0lx\n", target_phy, phdr->p_vaddr);

        EFI_MEMORY_DESCRIPTOR* md = efi_info.memory_descriptors + efi_info.descriptor_size * efi_info.descriptor_count;
        efi_info.descriptor_count++;
        md->Type = EfiBootServicesData;
        md->Pad = 0;
        md->PhysicalStart = (u64)target_phy;
        md->VirtualStart = phdr->p_vaddr;
        // ceil(memsz/4096)
        md->NumberOfPages = (phdr->p_memsz + 4095) / 4096;
        // noncacheable+cachable+wt-able+wb-able
        md->Attribute = 0xf;

        copy(target_phy, buffer + phdr->p_offset, phdr->p_memsz);
      } else {
        Print(L"\n");
        Print(L"Don't know how to load sections that has filesize > memsize");
        return 0;
      }
    }
  }
  EntrypointFunc entrypoint = ehdr->e_entry;
  char *kernelEntry = phyKernelStart + ((uint64_t)entrypoint - KERNEL_START);

  Print(L"kernel phy start 0x%lx\n", phyKernelStart);
  efi_info.kernel_physical_start = (uint64_t)phyKernelStart;
  efi_info.kernel_physical_size = kernel_max_size;

  void dump_memory_map(char *memory_descriptors, uint64_t descriptor_version, uint64_t descriptor_size, uint64_t descriptor_count);
  dump_memory_map(efi_info.memory_descriptors, efi_info.descriptor_version, efi_info.descriptor_size, efi_info.descriptor_count);

  Print(L"Kernel entry: ");
  for (int i = 0; i < 16; i++) {
    Print(L"%02x ", (u8)((kernelEntry[i])));
  }
  Print(L"\nkernel loaded at virt: 0x%lx, phy: 0x%lx\n", entrypoint, kernelEntry);
  return entrypoint;
}

void hello() {
  Print(L"Kernel hello\n");
}

void my_print(const uint16_t *fmt, ...) {
  efi_call2(TextOut->OutputString, TextOut, fmt);
  efi_call2(TextOut->OutputString, TextOut, L"\r\n");

}

void dump_memory_map(char *memory_descriptors, uint64_t descriptor_version, uint64_t descriptor_size, uint64_t descriptor_count) {
  Print(L"Descriptor version/size/count: 0x%lx/0x%lx/0x%lx\n",
        descriptor_version,
        descriptor_size,
        descriptor_count);
  Print(L"Conventional memory regions:\n");
  Print(L"  Type Pad PhysicalStart VirtualStart NPages Attr\n");
  for (int i = 0; i < descriptor_count; i++) {
    EFI_MEMORY_DESCRIPTOR *md = (&memory_descriptors[0] + descriptor_size*i);
    if (md->Type != EfiReservedMemoryType) {
//      if (md->Type == EfiConventionalMemory/* && md->PhysicalStart == 0x100000*/) {
        Print(L"  0x%02lx 0x%08lx 0x%016lx 0x%016lx 0x%08lx 0x%08lx\n",
              md->Type,
              md->Pad,
              md->PhysicalStart,
              md->VirtualStart,
              md->NumberOfPages,
              md->Attribute);
//      }
    }
  }
}

void print_memory_map(EFI_SYSTEM_TABLE *SystemTable) {
  UINTN memoryMapSize = sizeof(efi_info.memory_descriptors);
  UINTN mapKey;
  EFI_STATUS st = efi_call5(SystemTable->BootServices->GetMemoryMap, &memoryMapSize, &efi_info.memory_descriptors[0], &mapKey, &efi_info.descriptor_size, &efi_info.descriptor_version);
  if (st != EFI_SUCCESS) {
    Print(L"Failed to GetMemoryMap the second time 0x%lx\n", st);
    return st;
  }
  efi_info.descriptor_count = memoryMapSize / efi_info.descriptor_size;

  dump_memory_map(&efi_info.memory_descriptors[0], efi_info.descriptor_version, efi_info.descriptor_size, efi_info.descriptor_count);
}

EFI_STATUS
efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
  InitializeLib(ImageHandle, SystemTable);


  // init console
  EFI_STATUS st = LibLocateProtocol(&TextOutProtocol, (VOID **) &TextOut);
  if (EFI_ERROR(st)) {
    Print(L"Failed to get SIMPLE TEXT OUT PROTOCOL\n");
    return st;
  }
  efi_call2(TextOut->OutputString, TextOut, L"TextOut->OutputString()\r\n");

  Print(L"Hello, world!\n");

//   egInitScreen();
//   Print(L"Screen initialized %lux%lu\n", egScreenWidth, egScreenHeight);
//   egClearScreen();

  uint64_t result = get_cs();
  Print(L"cs: %04lx\n", result);

  EFI_PHYSICAL_ADDRESS PhysicalBuffer;
  UINT64 AllocSize = (1 + 32) * 1024 * 1024;
  UINTN Pages = AllocSize / 4096;
  EFI_STATUS status = efi_call4(
      SystemTable->BootServices->AllocatePages,
      AllocateAnyPages,
      EfiBootServicesData,
      Pages,
      &PhysicalBuffer
  );
  if (status != EFI_SUCCESS) {
    Print(L"Failed to AllocatePages\n");
    return status;
  }

  Print(L"Allocated 0x%lx pages starting from 0x%lx\n", Pages, PhysicalBuffer);

  UINT64 stackStart = PhysicalBuffer;
  UINT64 stackSize = 1024 * 1024;
  UINT64 kernelStart = stackStart + stackSize;
  char *Buffer = (char*)kernelStart;
  UINT64 KernelMaxSize = AllocSize - stackSize;

  print_memory_map(SystemTable);

  // Get File system info
  status = LibLocateProtocol(&SimpleFileSystemProtocolGuid, (VOID**)&SimpleFileSystem);
  EFI_FILE_PROTOCOL *File;
  status = efi_call2(SimpleFileSystem->OpenVolume,SimpleFileSystem, &File);
  if (status != EFI_SUCCESS) {
    Print(L"Failed to open volume 0x%lx\n", status);
    return 1;
  }
  EFI_GUID infoid = EFI_FILE_SYSTEM_INFO_ID;
  uint64_t GetInfoBufferSize = KernelMaxSize;
  status = efi_call4(File->GetInfo, File, &infoid, &GetInfoBufferSize, Buffer);
  if (status != EFI_SUCCESS) {
    Print(L"Failed to getInfo 0x%lx\n", status);
    return 1;
  }

  EFI_FILE_SYSTEM_INFO  *fs_info = Buffer;
  Print(L"VolumeSize=0x%0lx, FreeSpace:0x%0lx, BlockSize=0x%0x, Label=%s\n", fs_info->VolumeSize, fs_info->FreeSpace, fs_info->BlockSize, fs_info->VolumeLabel);

  EFI_FILE_PROTOCOL *FileHandle;
  status = efi_call5(File->Open, File, &FileHandle, L"kernel", EFI_FILE_MODE_READ, 0);
  if (status != EFI_SUCCESS) {
    Print(L"Failed to open kernel file\n");
    return 1;
  }

  EFI_GUID file_info_id = EFI_FILE_INFO_ID;
  uint64_t GetFileInfoBufferSize = KernelMaxSize;
  status = efi_call4(File->GetInfo, FileHandle, &file_info_id, &GetFileInfoBufferSize, Buffer);
  if (status != EFI_SUCCESS) {
    Print(L"Failed to get file info 0x%lx\n", status);
    return 1;
  }

  EFI_FILE_INFO  *f_info = Buffer;
  Print(L"Size=0x%0lx, Name='%s'\n", f_info->FileSize, f_info->FileName);

  // read file
  UINT64 BufferSize = KernelMaxSize;
  status = efi_call3(File->Read, FileHandle, &BufferSize, Buffer);
  if (status != EFI_SUCCESS) {
    Print(L"Failed to read kernel file st=%ld\n", status);
    return 1;
  }
  Print(L"Kernel file size 0x%0lx\n", BufferSize);

  EntrypointFunc entrypoint = LoadKernel(SystemTable, kernelStart, KernelMaxSize, Buffer, BufferSize);
  if (!entrypoint) {
    Print(L"Failed to load kernel\n");
    return 1;
  }

  {
    u8 memory_descriptors[8192];
    u64 descriptor_size;
    u64 descriptor_count;
    u64 descriptor_version;

    UINTN memoryMapSize = sizeof(memory_descriptors);
    UINTN mapKey;
    status = efi_call5(SystemTable->BootServices->GetMemoryMap, &memoryMapSize, &memory_descriptors[0], &mapKey, &descriptor_size, &descriptor_version);
    if (status != EFI_SUCCESS) {
      Print(L"Failed to GetMemoryMap the second time 0x%lx\n", status);
      return status;
    }
    descriptor_count = memoryMapSize / descriptor_size;

    status = efi_call2(SystemTable->BootServices->ExitBootServices, ImageHandle, mapKey);
    if (status != EFI_SUCCESS) {
      Print(L"Failed to ExitBootServices 0x%lx\n", status);
      return status;
    }
  }

  status = efi_call4(
      SystemTable->RuntimeServices->SetVirtualAddressMap,
      efi_info.descriptor_count,
      efi_info.descriptor_size,
      efi_info.descriptor_version,
      efi_info.memory_descriptors);
  if (status != EFI_SUCCESS) {
    puts("failed set memory map\n");
    return 1;
  }

  puts("hello serial from bootloader\n");

  setup_kernel_image_page_table((void*)kernelStart);

  puts("kernel page table setup done\n");

  asm volatile(
      "movq %1, %%rdi \n\t"
      "movq %0, %%rax \n\t"
      "jmp %%rax \n\t"
      :
      :"r"(entrypoint), "r"(&efi_info)
      :
  );

  return 0;
}
