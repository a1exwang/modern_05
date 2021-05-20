#pragma once
#include <device/pci.h>
#include <common/kstring.hpp>

class AHCIDriver : public PCIDeviceDriver {
 public:
  virtual kstring name() const {
    return "AHCI";
  }
  virtual bool Enumerate(PCIDeviceInfo *info) override;
  virtual bool HandleInterrupt(u64 irq_num) override { return false; }

  void Init();

 private:
  u32 *abar;
};
