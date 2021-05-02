#pragma once

#include <lib/defs.h>

#ifdef __cplusplus
typedef u64 (*print_function)(const wchar_t *fmt, ...);
#else
typedef u64 (*print_function)(const u16 *fmt, ...);
#endif
#pragma pack(push, 1)
struct EFIServicesInfo {
  u64 phy_size;
};
#pragma pack(pop)

constexpr u64 EFIServiceInfoAddress = 0x100000;