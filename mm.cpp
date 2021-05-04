#include <mm.h>
#include <lib/defs.h>
#include <lib/serial_port.h>
#include <lib/string.h>
#include <kernel.h>
#include <efi/efi.h>
#include <efi/efidef.h>

#define CR4_PSE (1u<<4u)
#define CR4_PAE (1u<<5u)

static bool is_page_present(u64 page_table_entry) {
  return page_table_entry & 0x1u;
}

u64 global_test = 0x233;

void calculate_total_pages(const u64* pml4t) {
  Kernel::sp() << "cr3 " << (u64)pml4t << "\n";

  // 9 | 9 | 9 | 9 | 12
  u64 total_pages = 0;
  bool pde_printed = false;
  bool pte_printed = false;
  for (int i = 0; i < 512; i++) {
    if (is_page_present(pml4t[i])) {
      Kernel::sp() << "pml4e " << i << " " << pml4t[i] << "\n";
      auto pdpt = reinterpret_cast<const u64*>(pml4t[i] & 0xfffffffffffff000);
      for (int j = 0; j < 512; j++) {
        if (is_page_present(pdpt[j])) {
          if (i == 0 && j == 0) {
            Kernel::sp() << "pdpe " << j << " " << pdpt[j] << "\n";
          }
          auto pdt = reinterpret_cast<const u64*>(pdpt[j] & 0xfffffffffffff000);
          for (int k = 0; k < 512; k++) {
            if (is_page_present(pdt[k])) {
//              Kernel::sp() << "pde " << k << " " << pdt[k] << "\n";

              // 2M page?
              if (pdt[k] & 0x80) {
                // 2m page
                u64 virtual_page_id = (((i*512+j)*512)+k);
                u64 phy_page_id = pdt[k] >> 21;

//                Kernel::sp() << "page map " << virtual_page_id << " -> " << phy_page_id << "\n";
              } else {
                // 4k page
                auto pt = reinterpret_cast<const u64*>(pdt[k] & 0xfffffffffffff000);
                for (int l = 0; l < 512; l++) {
                  if (is_page_present(pt[l])) {
                    if (!pte_printed) {
                      u64 virtual_page_id = (((i*512+j)*512)+k)*512+l;
                      u64 phy_page_id = pt[l] >> 12;
//                      Kernel::sp() << "pte " << l << " " << pt[l] << "\n";
                      pte_printed = true;
                      break;
                    }
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
  Kernel::sp() << "total pages: 0x" << SerialPort::IntRadix::Hex << total_pages << '\n';
}

u8 kernel_gdt[0x1000];

void gdt_init() {
  auto [gdt, limit] = get_gdt();
  memcpy(kernel_gdt, gdt, limit+1);
  load_gdt(std::make_tuple(kernel_gdt, sizeof(kernel_gdt)-1));
}

//extern "C" void setup_kernel_image_page_table();

void page_table_init() {
  Kernel::sp() << "setting up page table \n";
//  setup_kernel_image_page_table();
  auto kernel_addr = ((u64)&global_test + 0xffff800000000000ul);
  Kernel::sp() << "end test, accessing kernal space address *(" << kernel_addr << ") = " << *(u64*)kernel_addr << "\n";
}

struct PhysicalMemorySection {
  u64 phy;
  u64 n_pages;
};

PhysicalMemorySection available_memory[1024];
u64 n_available_memory_sections;

void dump_efi_info() {
  n_available_memory_sections = 0;
  auto &efi_info = Kernel::k->efi_info;

  u64 total_size = 0;
  Kernel::sp() << "Memory segments: \n";
  for (int i = 0; i < efi_info.descriptor_count; i++) {
    auto md = (EFI_MEMORY_DESCRIPTOR*)(efi_info.memory_descriptors + i * efi_info.descriptor_size);
    if (md->Type == EfiConventionalMemory) {
      auto l1 = md->PhysicalStart;
      auto r1 = md->PhysicalStart + md->NumberOfPages*4096;

      auto l2 = efi_info.kernel_physical_start;
      auto r2 = efi_info.kernel_physical_start + efi_info.kernel_physical_size;

      if (l1 >= r2 || l2 >= r1) {
        // ok, no intersection
      } else {
        Kernel::k->panic("Kernel image should not intersection with conventional memory\n");
      }

      available_memory[n_available_memory_sections].n_pages = md->NumberOfPages;
      available_memory[n_available_memory_sections].phy = md->PhysicalStart;
      n_available_memory_sections++;
      total_size += md->NumberOfPages*4096;
    }
  }

  Kernel::sp() << "Available memory sections = " << n_available_memory_sections << ", size = " << SerialPort::IntRadix::Dec << total_size/1024 << "KiB\n";
}

void mm_init() {
  u64 cr3 = 0;
  asm volatile("mov %%cr3,%0" : "=r" (cr3));
  auto pml4t = reinterpret_cast<const u64*>(cr3 & 0xfffffffffffff000);
  calculate_total_pages(pml4t);

  gdt_init();

  dump_efi_info();

//  page_table_init();
}