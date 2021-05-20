#include <cpu_defs.h>
#include <kernel.h>
#include <device/pci.h>
#include <lib/string.h>

const char *ACPI_TABLE_SIGNATURE_PCIE_CONFIG = "MCFG";
const char *ACPI_TABLE_SIGNATURE_APIC = "APIC";

void efi_table_init() {
  auto xsdt_phy = Kernel::k->efi_info.xsdt_phy_addr;
  assert(xsdt_phy != 0, "");
  assert(xsdt_phy < IDENTITY_MAP_PHY_END, "XSDT out of range");

  auto xsdt = (SystemDescriptionTable *)(xsdt_phy + KERNEL_START);
  assert(memcmp(xsdt->signature, "XSDT", 4) == 0, "Invalid root xsdt, signature not match");
  auto n_entries = (xsdt->length - sizeof(SystemDescriptionTable)) / 8;

  Kernel::sp() << IntRadix::Dec << "ACPI extended system description table size = " << xsdt->length << ", entries = " << n_entries << "\n";
  auto tables_phy = (u64*)&xsdt->data;
  SystemDescriptionTable *pcie_sdt = nullptr;
  SystemDescriptionTable *apic_sdt = nullptr;
  for (int i = 0; i < n_entries; i++) {
    auto phy = tables_phy[i];
    if (phy < IDENTITY_MAP_PHY_END) {
      auto sdt = (SystemDescriptionTable*)(phy + KERNEL_START);
      char sig[5] = {0};
      memcpy(sig, sdt->signature, 4);
      Kernel::sp() << "  ACPI table " << i << " " << (const char*)sig << "\n";
      if (memcmp(sdt->signature, ACPI_TABLE_SIGNATURE_PCIE_CONFIG, 4) == 0) {
        pcie_sdt = sdt;
      } else if (memcmp(sdt->signature, ACPI_TABLE_SIGNATURE_APIC, 4) == 0) {
        apic_sdt = sdt;
      }
    }
  }

  // IOAPIC table
  if (apic_sdt == nullptr) {
    Kernel::k->panic("Cannot find APIC SDT");
  }
  void ioapic_init(SystemDescriptionTable*);
  ioapic_init(apic_sdt);

  // PCI Express TABLE
  if (pcie_sdt == nullptr) {
    Kernel::k->panic("Cannot find PCIE Config SDT");
  }
  u64 n = (pcie_sdt->length-sizeof(SystemDescriptionTable)) / 16;
  auto segment_groups = (ECMGroup*)((char*)&pcie_sdt->data+8);
  Kernel::k->pci_bus_driver_->Enumerate(segment_groups, n);
}

