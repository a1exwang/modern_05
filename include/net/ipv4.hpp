#pragma once

#pragma pack(push, 1)
struct IPv4Address {
  IPv4Address(u32 ip) :d(ip) {}

  union {
    u32 d;
    u8 b[4];
  };

  bool operator<(const IPv4Address &rhs) const {
    return d < rhs.d;
  }
};
#pragma pack(pop)