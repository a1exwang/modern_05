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
void puti(u64 i);
void puts(const char *s);

void dump_pagetable(const u64* pml4t) {
  // 9 | 9 | 9 | 9 | 12
  u64 total_pages = 0;
  int pde_printed = 0;
  int pte_printed = 0;
  for (u64 i = 0; i < 512; i++) {
    if (pml4t[i] & 0x1) {
//      Kernel::sp() << "pml4e " << i << " " << pml4t[i] << "\n";
      puts("pml4e "); puti(i); puts(" "); puti(pml4t[i]); puts("\n");
      u64* pdpt = (u64*)(pml4t[i] & 0xfffffffffffff000);
      for (u64 j = 0; j < 512; j++) {
        if (pdpt[j] & 0x1) {
          u64* pdt = (u64*)(pdpt[j] & 0xfffffffffffff000);
          puts("  pdpe "); puti(i*512+j); puts(" "); puti(pdpt[j]); puts("\n");
          for (u64 k = 0; k < 512; k++) {
            if (pdt[k] & 0x1) {
//              Kernel::sp() << "pde " << k << " " << pdt[k] << "\n";

              // 2M page?
              if (pdt[k] & 0x80) {
//                puts("    map "); puti(((i*512+j)*512+k) << 21); puts(" "); puti(pdt[k]); puts("\n");
                // 2m page
                u64 virtual_page_id = (((i*512+j)*512)+k);
                u64 phy_page_id = pdt[k] >> 21;
              } else {
//                puts("    pde "); puti((i*512+j)*512+k); puts(" "); puti(pdt[k]); puts("\n");
                // 4k page
                u64* pt = (u64*)(pdt[k] & 0xfffffffffffff000);
                for (u64 l = 0; l < 512; l++) {
                  if (pt[l] & 0x1) {
//                    puts("      pte "); puti(l); puts(" "); puti(pt[l]); puts("\n");

                    u64 virtual_page_id = (((i*512+j)*512)+k)*512+l;
                    u64 phy_page_id = pt[l] >> 12;
                    total_pages++;
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}


void setup_kernel_image_page_table(u64 kernelPhyStart) {
  struct PageMappingL4Entry* cr3 = 0;
  asm volatile("mov %%cr3, %0" :"=r"(cr3));

  for (u64 i = 0; i < sizeof(struct KernelImagePageTable); i++) {
    ((u8*)&t)[i] = 0;
  }

  t.pml4t[0] = cr3[0];

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

//  for (u64 i = 0; i < sizeof(t.pt) / sizeof(t.pt[0]); i++) {
//    t.pt[i].p = 1;
//    t.pt[i].rw = 1;
//    t.pt[i].base_addr = i;
//  }

  // only map the first 64MiB for kernel image file
  // other kernel space address is not mapped
  for (u64 i = 0; i < (64*1024*1024 / 4096); i++) {
    t.pt[i].p = 1;
    t.pt[i].rw = 1;
    t.pt[i].base_addr = (kernelPhyStart/4096) + i;
  }

  cr3 = t.pml4t;
  puts("before setting cr3 to ");
  puti((u64)cr3);
  puts("\n");
  asm volatile("movq %0, %%cr3" : :"r"(cr3));
  dump_pagetable(t.pml4t);
}

