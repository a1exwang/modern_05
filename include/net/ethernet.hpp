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
};
#pragma pack(pop)

class EthernetDriver {
 public:
  virtual void Tx(EthernetAddress dst, u16 protocol, u8 *data, size_t size) = 0;
};
