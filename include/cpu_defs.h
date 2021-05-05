#pragma once

#include <lib/defs.h>

#define PAGE_SIZE 4096UL
#define ALIGN(N) __attribute__ ((aligned (N)))
#define PAGES_PER_TABLE 512UL
#define KERNEL_IMAGE_PAGES ((512UL*512UL))
#define KERNEL_START (0xffff800000000000UL)
#define IDENTITY_MAP_SIZE (1024UL*1024UL*1024UL - KERNEL_IMAGE_PAGES*PAGE_SIZE)
#define IDENTITY_MAP_START (KERNEL_START+KERNEL_IMAGE_PAGES*PAGE_SIZE)
#define IDENTITY_MAP_END (IDENTITY_MAP_START+IDENTITY_MAP_SIZE)

#pragma pack(push, 1)
struct SegmentDescriptor {
  u16 __segment_limit_lo16; // unused
  u16 __base_addr_lo16; // unused
  u8 __base_addr_mid8; // unused

  u8 __accessed : 1; // unused
  u8 __wr : 1; // unused
  u8 _dc : 1; // only code, please set to 0
  u8 __exec : 1; // unused, 1 for code segment
  u8 __reserved : 1; // unused == 1
  u8 _dpl : 2;// only for code segment
  u8 present : 1;

  u8 __segment_limit_hi4 : 4; // unused

  u8 __avl : 1; // unused
  u8 _long_mode : 1; // only code, set to 1
  u8 _default_operand_size : 1; // only code, set to 0
  u8 __granularity : 1; // unused

  u8 __base_addr_hi8; // unused
};

enum DescriptorType {
  LongLDT = 0x2,
  LongAvailableTSS = 0x9,
  LongBusyTSS = 0xb,
  LongCallGate = 0xc,
  LongInterruptGate = 0xe,
  LongTrapGate = 0xf,
};

struct InterruptDescriptor {
  u16 offset_lo16;
  u16 selector;

  u8 ist : 4;
  u8 zero : 4;
  u8 type : 4;

  u8 zero2 : 1;
  u8 dpl : 2;
  u8 p : 1;

  u16 offset_mid16;
  u32 offset_hi32;
  u32 zero3;
};

struct PageTableEntry {
  u8 p : 1;
  u8 rw : 1;
  u8 us : 1;
  u8 pwt : 1;
  u8 pcd : 1;
  u8 a : 1;
  u8 d : 1;
  u8 pat : 1;

  u8 g : 1;
  u8 avl : 3;
  u64 base_addr : 40;
  u8 available : 7;
  u8 reserved : 4;
  u8 nx : 1;
};

struct PageDirectoryEntry {
  u8 p : 1;
  u8 rw : 1;
  u8 us : 1;
  u8 pwt : 1;
  u8 pcd : 1;
  u8 a : 1;
  u8 ignore1 : 1;
  u8 zero1 : 1;

  u8 ignore2: 1;
  u8 avl : 3;
  u64 base_addr : 40;
  u16 available : 11;
  u8 nx : 1;
};
struct PageDirectoryPointerEntry {
  u8 p : 1;
  u8 rw : 1;
  u8 us : 1;
  u8 pwt : 1;
  u8 pcd : 1;
  u8 a : 1;
  u8 ignore1 : 1;
  u8 zero1 : 1;

  u8 ignore2: 1;
  u8 avl : 3;
  u64 base_addr : 40;
  u16 available : 11;
  u8 nx : 1;
};
struct PageMappingL4Entry {
  u8 p : 1;
  u8 rw : 1;
  u8 us : 1;
  u8 pwt : 1;
  u8 pcd : 1;
  u8 a : 1;
  u8 ignore1 : 1;
  u8 zero1 : 1;

  u8 zero2: 1;
  u8 avl : 3;
  u64 base_addr : 40;
  u16 available : 11;
  u8 nx : 1;
};
#pragma pack(pop)
