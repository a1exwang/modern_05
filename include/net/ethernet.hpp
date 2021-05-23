#pragma once

#include <cpu_defs.h>
#include <lib/string.h>
#include <kernel.h>
#include <common/hexdump.hpp>
#include <common/vdtor.hpp>

constexpr u16 EtherTypeIPv4 = 0x0800;
constexpr u16 EtherTypeARP = 0x0806;

#pragma pack(push, 1)
struct EthernetAddress {
  u8 data[6];

  bool operator<(const EthernetAddress &rhs) const {
    return memcmp(data, rhs.data, sizeof(data)) < 0;
  }

  int operator<=>(const EthernetAddress &rhs) const {
    return memcmp(data, rhs.data, sizeof(data));
  }

  bool operator==(const EthernetAddress &rhs) const {
    return memcmp(data, rhs.data, sizeof(data)) == 0;
  }
  bool operator!=(const EthernetAddress &rhs) const {
    return !(*this == rhs);
  }

  void print() const {
    for (int i = 0; i < sizeof(data); i++) {
      if (i != 0) {
        Kernel::sp() << ":";
      }
      hex01(data[i]);
    }
  }

  bool IsBroadcast() const {
    for (auto v : data) {
      if (v != 0xff) {
        return false;
      }
    }
    return true;
  }

  static EthernetAddress Broadcast() {
    EthernetAddress ret;
    memset(ret.data, 0xff, sizeof(ret.data));
    return ret;
  }
};
struct KEthernetPacket {
  size_t size; // the payload size, does not include header
  size_t max_size; // the max payload size, does not include header

  // start of ethernet packet
  u8 pkt_start[0];
  EthernetAddress dst;
  EthernetAddress src;
  u16 protocol;
  u8 data[];
};
constexpr size_t EthernetHeaderSize = sizeof(EthernetAddress) * 2 + sizeof(u16);
#pragma pack(pop)

#define L2MTU 1500
static KEthernetPacket *create_packet() {
  auto ret = (KEthernetPacket*)kmalloc(sizeof(KEthernetPacket) + L2MTU);
  ret->size = 0;
  ret->max_size = L2MTU;
  return ret;
}

class IPDriver;
class EthernetDriver {
 public:
  virtual bool TxEnqueue(EthernetAddress dst, u16 protocol, u8 *data, size_t size) = 0;
  virtual EthernetAddress address() const = 0;
  virtual void SetIPDriver(IPDriver *driver) = 0;
};
