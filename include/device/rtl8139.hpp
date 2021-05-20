#pragma once
#include <device/pci.h>

class RTL8139Driver : public PCIDeviceDriver {
 public:
  RTL8139Driver() { }
  kstring name() const override {
    return "rtl8139";
  }
  bool Enumerate(PCIDeviceInfo *info) override;
  bool HandleInterrupt(u64 irq_num) override;
};
