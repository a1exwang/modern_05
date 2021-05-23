#pragma once
#include <device/pci.h>
#include <net/ethernet.hpp>

class RTL8139Driver : public PCIDeviceDriver, public EthernetDriver {
 public:
  RTL8139Driver() { }
  kstring name() const override {
    return "rtl8139";
  }
  bool Enumerate(PCIDeviceInfo *info) override;
  bool HandleInterrupt(u64 irq_num) override;
  void Tx(EthernetAddress dst, u16 protocol, u8 *data, size_t size) override;
};
