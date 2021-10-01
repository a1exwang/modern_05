#pragma once
#include <device/pci.h>
#include <common/kstring.hpp>
#include <common/error.hpp>

constexpr u64 SectorSize = 512;

class AHCIDevice {
 public:
  explicit AHCIDevice(void *port_control_reg):port_control_reg(port_control_reg) {}

  // start, size are in bytes, they do not have to be aligned
  // temp buffer are allocated and data is then copied to output
  Error Read(void *output, u64 start, u64 size);
 private:
  void *port_control_reg;
};

class AHCIDriver : public PCIDeviceDriver {
 public:
  virtual kstring name() const {
    return "AHCI";
  }
  virtual bool Enumerate(PCIDeviceInfo *info) override;
  virtual bool HandleInterrupt(u64 irq_num) override { return false; }

  static void KthreadEntry(AHCIDriver *that);

  void Init();
 private:
  kvector<AHCIDevice> sata_devices;
  u32 *abar;
};
