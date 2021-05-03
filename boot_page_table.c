#include "lib/defs.h"

#pragma pack(push, 1)
struct PageTableEntry {
  u8 p : 1;
  u8 rw : 1;
  u8 us : 1;
  u8 pwt : 1;
  u8 pcd : 1;
  u8 a : 1;
  u8 d : 1;
  u8 pat : 1;

  u8 g : 1;
  u8 avl : 3;
  u64 base_addr : 40;
  u8 available : 7;
  u8 reserved : 4;
  u8 nx : 1;
};

struct PageDirectoryEntry {
  u8 p : 1;
  u8 rw : 1;
  u8 us : 1;
  u8 pwt : 1;
  u8 pcd : 1;
  u8 a : 1;
  u8 ignore1 : 1;
  u8 zero1 : 1;

  u8 ignore2: 1;
  u8 avl : 3;
  u64 base_addr : 40;
  u16 available : 11;
  u8 nx : 1;
};
struct PageDirectoryPointerEntry {
  u8 p : 1;
  u8 rw : 1;
  u8 us : 1;
  u8 pwt : 1;
  u8 pcd : 1;
  u8 a : 1;
  u8 ignore1 : 1;
  u8 zero1 : 1;

  u8 ignore2: 1;
  u8 avl : 3;
  u64 base_addr : 40;
  u16 available : 11;
  u8 nx : 1;
};
struct PageMappingL4Entry {
  u8 p : 1;
  u8 rw : 1;
  u8 us : 1;
  u8 pwt : 1;
  u8 pcd : 1;
  u8 a : 1;
  u8 ignore1 : 1;
  u8 zero1 : 1;

  u8 zero2: 1;
  u8 avl : 3;
  u64 base_addr : 40;
  u16 available : 11;
  u8 nx : 1;
};
#pragma pack(pop)

#define PAGE_SIZE 4096
#define ALIGN(N) __attribute__ ((aligned (N)))
#define PAGES_PER_TABLE 512
#define KERNEL_IMAGE_PAGES ((512*512))

// map 0-64M -> KERNEL_START -> KERNEL_START+64M
struct KernelImagePageTable {
  struct PageMappingL4Entry pml4t[1 * PAGES_PER_TABLE] ALIGN(PAGE_SIZE);
  struct PageDirectoryPointerEntry pdpt[1 * PAGES_PER_TABLE] ALIGN(PAGE_SIZE);
  struct PageDirectoryEntry pdt[1 * PAGES_PER_TABLE] ALIGN(PAGE_SIZE);
  struct PageTableEntry pt[KERNEL_IMAGE_PAGES] ALIGN(PAGE_SIZE);
};

struct KernelImagePageTable t ALIGN(PAGE_SIZE);

void setup_kernel_image_page_table() {
  for (u64 i = 0; i < sizeof(struct KernelImagePageTable); i++) {
    ((u8*)&t)[i] = 0;
  }
//  memset(&t, 0, sizeof(KernelImagePageTable));

  t.pml4t[0].p = 1;
  t.pml4t[0].rw = 1;
  t.pml4t[0].base_addr = ((u64)(&t.pdpt)) >> 12;

  t.pml4t[256].p = 1;
  t.pml4t[256].rw = 1;
  t.pml4t[256].base_addr = ((u64)(&t.pdpt)) >> 12;

  t.pdpt[0].p = 1;
  t.pdpt[0].rw = 1;
  t.pdpt[0].base_addr = ((u64)(&t.pdt)) >> 12;

  for (u64 i = 0; i < KERNEL_IMAGE_PAGES / PAGES_PER_TABLE; i++) {
    t.pdt[i].p = 1;
    t.pdt[i].rw = 1;
    t.pdt[i].base_addr = ((u64)&t.pt[PAGES_PER_TABLE * i]) >> 12;
  }

  for (u64 i = 0; i < sizeof(t.pt) / sizeof(t.pt[0]); i++) {
    t.pt[i].p = 1;
    t.pt[i].rw = 1;
    t.pt[i].base_addr = i;
  }

  void* cr3 = &t.pml4t;

//  Kernel::sp() << "before cr3 " << (u64)cr3 << "\n";
  asm volatile("mov %0, %%cr3" : :"r"(cr3));
}
