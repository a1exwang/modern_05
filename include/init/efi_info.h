#pragma once

#include <lib/defs.h>

#pragma pack(push, 1)
struct EFIServicesInfo {
  u8 memory_descriptors[8192];
  u64 descriptor_size;
  u64 descriptor_count;
  u64 descriptor_version;

  u64 kernel_physical_start;
  u64 kernel_physical_size;
};
#pragma pack(pop)