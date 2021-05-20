#include <device/pci_devices.hpp>
#include <device/pci.h>
#include <device/rtl8139.hpp>
#include <device/ahci.hpp>

void register_pci_devices(PCIBusDriver &driver) {
  // Realtek RTL8139 10M/100M Ethernet Adapter
  driver.RegisterDriver<RTL8139Driver>(0x10ec, 0x8139);

  // Intel ICH9 AHCI Host Controller
  // In qemu
  driver.RegisterDriver<AHCIDriver>(0x8086, 0x2922);
  // In VirtualBox
  driver.RegisterDriver<AHCIDriver>(0x8086, 0x2829);
}
