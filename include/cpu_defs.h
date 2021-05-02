#pragma once

#include <lib/defs.h>

#pragma pack(push, 1)
struct SegmentDescriptor {
  u16 segment_limit_lo16;
  u16 base_addr_lo16;
  u8 base_addr_mid8;

  u8 accessed : 1;
  u8 wr : 1;
  u8 dc : 1;
  u8 exec : 1;
  u8 reserved : 1;
  u8 dpl : 2;
  u8 present : 1;

  u8 segment_limit_hi4 : 4;

  u8 avl : 1;
  u8 long_mode : 1;
  u8 default_operand_size : 1;
  u8 granularity : 1;

  u8 base_addr_hi8;
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
#pragma pack(pop)
