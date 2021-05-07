#include <cpu_defs.h>
#include <lib/port_io.h>
#include <kernel.h>
#include <device/pci.h>
#include <lib/string.h>
#include <mm/page_alloc.h>

u16 pci_config_read_word(u16 bus, u16 slot, u16 func, u16 offset){
  unsigned short tmp = 0;

  /* create configuration address as per Figure 1 */
  u32 address = (u32)(((u32)bus << 16) | ((u32)slot << 11) |
      ((u32)func << 8) | ((u32)offset & 0xfc) | ((u32)0x80000000));

  outl(0xCF8, address);

  /* (offset & 2) * 8) = 0 will choose the fisrt word of the 32 bits register */
  return (unsigned short)((inl(0xCFC) >> ((offset & 2) * 8)) & 0xffff);
}

constexpr u16 PCI_INVALID_VENDOR = 0xffff;
constexpr u16 PCI_INVALID_DEVICE = 0xffff;

#pragma pack(push, 1)
struct SystemDescriptionTable {
  char signature[4];
  u32 length;
  u8 revision;
  u8 checksum;
  char oem_id[6];
  u64 oem_table_id;
  u32 oem_revision;
  u32 creator_id;
  u32 creator_revision;
  u8 data[0];
};
struct ExtendedConfigSpace {
  u16 vendor;
  u16 device;

  u16 command;
  u16 status;

  u8 revision_id;
  u8 prog_if;
  u8 subclass;
  u8 class_code;

  u8 cache_line_size;
  u8 latency_timer;
  u8 header_type;
  u8 bist;

  u32 bars[6];

  u32 cardbus_cis_pointer;

  u16 subsystem_vendor;
  u16 subsystem_id;

  u32 expansion_rom_base_addr;

  u8 capabilities_pointer;
  u8 reserved[3];

  u8 reserved2[4];

  u8 interrupt_line;
  u8 interrupt_pin;
  u8 min_grant;
  u8 max_latency;
};
#pragma pack(pop)

const char *ACPI_TABLE_SIGNATURE_PCIE_CONFIG = "MCFG";

u32 *abar = nullptr;

void enumerate_pci_bus() {
  auto xsdt_phy = Kernel::k->efi_info.xsdt_phy_addr;
  assert(xsdt_phy != 0, "");
  assert(xsdt_phy < IDENTITY_MAP_PHY_END, "XSDT out of range");

  auto xsdt = (SystemDescriptionTable *)(xsdt_phy + KERNEL_START);
  assert(memcmp(xsdt->signature, "XSDT", 4) == 0, "Invalid root xsdt, signature not match");
  auto n_entries = (xsdt->length - sizeof(SystemDescriptionTable)) / 8;

  Kernel::sp() << IntRadix::Dec << "ACPI extended system description table size = " << xsdt->length << ", entries = " << n_entries << "\n";
  auto tables_phy = (u64*)&xsdt->data;
  SystemDescriptionTable *pcie_sdt = nullptr;
  for (int i = 0; i < n_entries; i++) {
    auto phy = tables_phy[i];
    if (phy < IDENTITY_MAP_PHY_END) {
      auto sdt = (SystemDescriptionTable*)(phy + KERNEL_START);
      char sig[5] = {0};
      memcpy(sig, sdt->signature, 4);
      Kernel::sp() << "  ACPI table " << i << " " << (const char*)sig << "\n";
      if (memcmp(sdt->signature, ACPI_TABLE_SIGNATURE_PCIE_CONFIG, 4) == 0) {
        pcie_sdt = sdt;
        break;
      }
    }
  }
  if (pcie_sdt == nullptr) {
    Kernel::k->panic("Cannot find PCIE Config SDT");
  }

#pragma pack(push, 1)
  struct ECMGroup {
    u64 base;
    u16 segment_group;
    u8 bus_start;
    u8 bus_end;
    u32 reserved;
  };
#pragma pack(pop)
  u64 n = (pcie_sdt->length-sizeof(SystemDescriptionTable)) / 16;
  auto segment_groups = (ECMGroup*)((char*)&pcie_sdt->data+8);

  Kernel::sp() << "Enumerating PCI devices:\n";
  for (u64 i = 0; i < n; i++) {
    Kernel::sp() << IntRadix::Hex << "Segment group " << segment_groups[i].base << " " << segment_groups[i].segment_group << segment_groups[i].bus_start << " " << segment_groups[i].bus_end << "\n";
    auto base = segment_groups[i].base;
    if (base >= IDENTITY_MAP_START) {
      Kernel::sp() << "Warning segment group start addr out of range " << IntRadix::Hex << base << "\n";
      continue;
    }


    for (int j = 0; j < segment_groups[i].bus_end - segment_groups[i].bus_start; j++) {
      auto bus = segment_groups[i].bus_start + j;
      for (int device = 0; device < 32; device++) {
        auto cs0 = (ExtendedConfigSpace*)(KERNEL_START + (base | (j << 20) | (device << 15)));
        if (cs0->vendor == PCI_INVALID_VENDOR) {
          continue;
        }

        Kernel::sp()
            << IntRadix::Hex
            << " "
            << " bus 0x" << bus
            << " slot 0x" << device << "\n";

        for (int function = 0; function < 8; function++) {
          auto cs = (ExtendedConfigSpace*)(KERNEL_START + (base | (j << 20) | (device << 15 | function << 12)));
          if (cs->vendor == PCI_INVALID_VENDOR) {
            continue;
          }
          Kernel::sp()
              << "    function 0x" << function
              << " vendor 0x" << cs->vendor
              << " device 0x" << cs->device
              << " header 0x" << cs->header_type
              << " class 0x" << cs->class_code
              << " subclass 0x" << cs->subclass
              << "\n";
          // Intel ICH9, other AHCI compatible devices should also work
          if (cs->vendor == 0x8086 && cs->device == 0x2922) {
            abar = &cs->bars[5];
          }
        }
      }
    }
  }
  Kernel::sp() << "PCI Enumeration done\n";

  if (!abar) {
    Kernel::k->panic("ICH9 HBA device is not found");
  }

}

void *port_register(void *ahci_reg_page, u8 port) {
  return ((char*)ahci_reg_page + 0x100) + 0x80 * port;
}

//https://forum.osdev.org/viewtopic.php?p=311182&sid=15cf6d354b08796bb45a1fb0fbadea0b#p311182
#define SATA_SIG_ATAPI  0xEB140101  // SATAPI drive

void ahci_init() {
  assert(abar != nullptr, "abar is null");

//  void *ahci_reg_page = kernel_page_alloc(16);
//  u32 ahci_phy = (u64)ahci_reg_page - KERNEL_START;
  // TODO: use allocate to allocate non-cachable memory range, currently hard code it
  u32 ahci_phy = 0xcabcd000;
  void *ahci_reg_page = (void*)(ahci_phy + KERNEL_START);

  *abar = ahci_phy;

  Kernel::sp() << "set ahci abar = " << IntRadix::Hex << ahci_phy << "\n";

  u32 pi = *(u32*)((char*)ahci_reg_page + 0xc);
  Kernel::sp() << "ports implemented = " << IntRadix::Hex << pi << "\n";
  Kernel::sp() << "version = " << IntRadix::Hex << *(u32*)((char*)ahci_reg_page + 0x10) << "\n";

  for (int i = 0; i < 32; i++) {
    if ((1 << i) & pi) {
      Kernel::sp() << "port implemented 0x" << IntRadix::Hex << i;
      auto port_control_reg = port_register(ahci_reg_page, i);
      auto sstatus = *(u32*)((char*)port_control_reg + 0x28);
      auto det = sstatus & 0xf;
      auto spd = (sstatus>>4) & 0xf;
      auto ipm = (sstatus>>8) & 0xf;
      if (!(det == 3 && spd != 0 && ipm == 1)) {
        Kernel::sp() << " no device\n";
        continue;;
      }
      auto sig = *(u32*)((char*)port_control_reg+0x24);
      Kernel::sp() << " ACTIVE device, sig = 0x" << IntRadix::Hex << sig << "\n";
      if (sig == SATA_SIG_ATAPI) {
        Kernel::sp() << "  ATAPI device\n";
      }
    }
  }



  while(1);
}

void pci_init() {
  enumerate_pci_bus();

  ahci_init();
}