#pragma once
#include <common/endian.hpp>
#include <lib/string.h>
#include <net/ethernet.hpp>

#pragma pack(push, 1)
struct IPv4Address {

  IPv4Address() { }
  IPv4Address(u8 *data4) {
    memcpy(b, data4, 4);
  }
  IPv4Address(u8 a, u8 b, u8 c, u8 d) :b{a, b, c, d} {}
  IPv4Address(const IPv4Address &rhs) {
    memcpy(b, rhs.b, sizeof(b));
  }
  IPv4Address& operator=(const IPv4Address &rhs) {
    if (this != &rhs) {
      memcpy(b, rhs.b, sizeof(b));
    }
    return *this;
  }

  void print() const {
    Kernel::sp() << IntRadix::Dec << (int)b[0] << "." << (int)b[1] << "." << (int)b[2] << "." << (int)b[3];
  }

  // network order
  u8 b[4];

  u32 to_u32() const {
    return be2cpu(*(u32*)b);
  }

  bool operator<(const IPv4Address &rhs) const {
    return to_u32() < to_u32();
  }
};
#pragma pack(pop)

class ArpDriver;
class IPDriver {
 public:
  IPDriver(EthernetDriver *eth, ArpDriver *arp) :ethernet_driver_(eth), arp_driver_(arp) {}
  IPv4Address address() const {
    // TODO: remove hardcode
    return IPv4Address{ 172, 20, 0, 3};
  }
  IPv4Address gateway_address() const {
    // TODO: remove hardcode
    return IPv4Address{ 172, 20, 0, 1};
  }
  void HandleRx(EthernetAddress dst, EthernetAddress src, u16 protocol, kvector<u8> data);
 private:
  EthernetDriver *ethernet_driver_;
  ArpDriver *arp_driver_;
};