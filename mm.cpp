#include <mm.h>
#include <lib/defs.h>
#include <lib/serial_port.h>
#include <kernel.h>

#define CR4_PSE (1u<<4u)
#define CR4_PAE (1u<<5u)

// 9 9 9 9
// 512G 1G 2M 4K
u64 pml4t[512] = {0};
u64 pdpt[512] = {0};
u64 pdt[512] = {0};
u64 pt[512] = {0};

// +0xffff8000 00000000 mapping
// 1M~33M -> kernel image
// 33M~40M -> image data structures/non swappable

// assert phy_start, phy_end aligned to 4k
void setup_kernel_page_table(u64 phy_start, u64 phy_end) {
  // kernel start + 1G
  pml4t[0] = pml4t[256] = reinterpret_cast<u64>(pdpt);

}

static bool is_page_present(u64 page_table_entry) {
  return page_table_entry & 0x1u;
}

void mm_init() {
  u64 cr3 = 0;
  asm volatile("mov %%cr3,%0" : "=r" (cr3));
  auto pml4t = reinterpret_cast<const u64*>(cr3 & 0xfffffffffffff000);
  // 9 | 9 | 9 | 9 | 12
  u64 total_pages = 0;
  for (int i = 0; i < 512; i++) {
    if (is_page_present(pml4t[i])) {
      auto pdpt = reinterpret_cast<const u64*>(pml4t[i] & 0xfffffffffffff000);
      for (int j = 0; j < 512; j++) {
        if (is_page_present(pdpt[j])) {
          auto pdt = reinterpret_cast<const u64*>(pdpt[i] & 0xfffffffffffff000);
          for (int k = 0; k < 512; k++) {
            if (is_page_present(pdt[k])) {
              auto pt = reinterpret_cast<const u64*>(pdt[k] & 0xfffffffffffff000);
              for (int l = 0; l < 512; l++) {
                if (is_page_present(pt[l])) {
                  total_pages++;
                }
              }
            }
          }
        }
      }
    }
  }
  Kernel::sp() << "total pages: 0x" << SerialPort::IntRadix::Hex << total_pages << '\n';
}