#pragma once
#include <device/pci.h>
#include <net/ethernet.hpp>

class IPDriver;
class RTL8139Driver : public PCIDeviceDriver, public EthernetDriver {
 public:
  RTL8139Driver() :ip_driver_(nullptr) {
    tx_packet_ = create_packet();
  }
  kstring name() const override {
    return "rtl8139";
  }
  bool Enumerate(PCIDeviceInfo *info) override;
  bool HandleInterrupt(u64 irq_num) override;
  bool TxEnqueue(EthernetAddress dst, u16 protocol, u8 *payload, size_t size) override;
  EthernetAddress address() const override;
  void SetIPDriver(IPDriver *driver) override {
    ip_driver_ = driver;
  }
 private:
  KEthernetPacket *tx_packet_;
  IPDriver *ip_driver_;
};
