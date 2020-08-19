#pragma once

#include <lib/defs.h>

#pragma pack(push, 1)
struct EFIServicesInfo {
  u64 phy_size;
};
#pragma pack(pop)

u64 EFIServiceInfoAddress = 0x101000;