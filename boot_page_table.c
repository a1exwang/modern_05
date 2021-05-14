#include "include/common/defs.h"
#include "include/cpu_defs.h"

// map 0-64M -> KERNEL_START -> KERNEL_START+64M
struct KernelImagePageTable {
  struct PageMappingL4Entry pml4t[1 * PAGES_PER_TABLE] ALIGN(PAGE_SIZE);
  struct PageDirectoryPointerEntry pdpt[1 * PAGES_PER_TABLE] ALIGN(PAGE_SIZE);
  struct PageDirectoryEntry pdt[1 * PAGES_PER_TABLE] ALIGN(PAGE_SIZE);
  struct PageTableEntry pt[KERNEL_SPACE_PAGES] ALIGN(PAGE_SIZE);
};

struct KernelImagePageTable t ALIGN(PAGE_SIZE);
void puti(u64 i);
void puts(const char *s);

void dump_pagetable(const u64* pml4t) {
  // 9 | 9 | 9 | 9 | 12
  u64 total_pages = 0;
//  int pde_printed = 0;
//  int pte_printed = 0;
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

  for (u64 i = 0; i < PAGES_PER_TABLE; i++) {
    t.pdt[i].p = 1;
    t.pdt[i].rw = 1;
    t.pdt[i].base_addr = ((u64)&t.pt[PAGES_PER_TABLE * i]) >> 12;
  }

  // [KERNEL_START, KERNEL_START+64MiB) -> kernel image file, non-identity mapping, not manaaged by allocator
  // [KERNEL_START+64MiB, KERNEL_START+1G) -> identity mapping, available physical pages in this range are managed by allocator
  for (u64 i = 0; i < KERNEL_SPACE_PAGES; i++) {
    t.pt[i].p = 1;
    t.pt[i].rw = 1;
    t.pt[i].base_addr = i;
  }
  for (u64 i = 0; i < KERNEL_IMAGE_PAGES; i++) {
    t.pt[i].p = 1;
    t.pt[i].rw = 1;
    t.pt[i].base_addr = (kernelPhyStart/4096) + i;
  }

  cr3 = t.pml4t;
  puts("before setting cr3 to ");
  puti((u64)cr3);
  puts("\n");
  asm volatile("movq %0, %%cr3" : :"r"(cr3));
  dump_pagetable((const u64*)t.pml4t);
}

