#pragma once

void pci_init();

constexpr u16 PCI_INVALID_VENDOR = 0xffff;
constexpr u16 PCI_INVALID_DEVICE = 0xffff;

#pragma pack(push, 1)
struct SystemDescriptionTable {
  char signature[4];
  u32 length;
  u8 revision;
  u8 checksum;
  char oem_id[6];
  u64 oem_table_id;
  u32 oem_revision;
  u32 creator_id;
  u32 creator_revision;
  u8 data[0];
};
struct ExtendedConfigSpace {
  u16 vendor;
  u16 device;

  u16 command;
  u16 status;

  u8 revision_id;
  u8 prog_if;
  u8 subclass;
  u8 class_code;

  u8 cache_line_size;
  u8 latency_timer;
  u8 header_type;
  u8 bist;

  // 0x10
  u32 bars[6];

  u32 cardbus_cis_pointer;

  u16 subsystem_vendor;
  u16 subsystem_id;

  u32 expansion_rom_base_addr;

  u8 capabilities_pointer;
  u8 reserved[3];

  u8 reserved2[4];

  u8 interrupt_line;
  u8 interrupt_pin;
  u8 min_grant;
  u8 max_latency;
};
#pragma pack(pop)
