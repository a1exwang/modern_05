#pragma once
#include <common/endian.hpp>

#pragma pack(push, 1)
struct IPv4Address {
  IPv4Address() :d(0) {}
  IPv4Address(u32 ip) :d(ip) {}
  IPv4Address(u8 a, u8 b, u8 c, u8 d) :b{a, b, c, d} {}

  union {
    u32 d;
    u8 b[4];
  };

  u32 network_order() const {
    return cpu2be(d);
  }

  bool operator<(const IPv4Address &rhs) const {
    return d < rhs.d;
  }
};
#pragma pack(pop)

class ArpDriver;
class IPDriver {
 public:
  IPDriver(EthernetDriver *eth, ArpDriver *arp) :ethernet_driver_(eth), arp_driver_(arp) {}
  IPv4Address address() const {
    // TODO: remove hardcode
    return IPv4Address{172, 20, 0, 3};
  }
 private:
  EthernetDriver *ethernet_driver_;
  ArpDriver *arp_driver_;
};