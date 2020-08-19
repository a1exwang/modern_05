#pragma once

#include <lib/defs.h>

typedef void (*HelloFunc)();
#pragma pack(push, 1)
struct EFIServicesInfo {
  u64 phy_size;
  HelloFunc func;
};
#pragma pack(pop)

u64 EFIServiceInfoAddress = 0x100000;