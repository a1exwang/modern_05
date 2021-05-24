#pragma once
#include <cpu_defs.h>
#include <common/kstring.hpp>
#include <common/kmap.hpp>
#include <common/kmemory.hpp>
#include <lib/string.h>
#include <common/kvector.hpp>
#include <common/vdtor.hpp>

void pci_init();


constexpr u16 IOAPIC_IRQ_START = 0x40;
constexpr u16 IOAPIC_IRQ_COUNT = 0x18;
constexpr u16 IOAPIC_ISA_IRQ_COM1 = 0x44;
constexpr u16 IOAPIC_PCI_IRQ_START = 0x50;
constexpr u16 IOAPIC_PCI_IRQ_COUNT = 0x8;

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

#pragma pack(push, 1)
struct ECMGroup {
  u64 base;
  u16 segment_group;
  u8 bus_start;
  u8 bus_end;
  u32 reserved;
};
#pragma pack(pop)

struct PCIBar {
  u64 start;
  size_t size;
  bool is_io;
};

struct PCIDeviceInfo {
  ExtendedConfigSpace *config_space;
  PCIBar bars[6];
};

class PCIDeviceDriver {
 public:
  virtual kstring name() const = 0;
  virtual bool Enumerate(PCIDeviceInfo *info) = 0;

  virtual bool HandleInterrupt(u64 irq_num) = 0;

  // Workaround: Without this implementation, the compiler generated destructor will call ::operator delete instead,
  // which we don't have.
  VIRTUAL_DTOR(PCIDeviceDriver);
};

class PCIBusDriver {
 public:
  void Enumerate(ECMGroup *segment_groups, size_t n);

  template <typename T, typename... Args>
  void RegisterDriver(u16 vendor, u16 device, Args&& ... args) {
    static_assert(std::is_base_of_v<PCIDeviceDriver, T>);
    assert1(drivers_.find({vendor, device}) == drivers_.end());
    auto obj = knew<T>(std::forward<Args>(args)...);
    drivers_.emplace(std::make_pair(std::make_tuple(vendor, device), obj));
  }

 private:
  kmap<std::tuple<u16, u16>, kup<PCIDeviceDriver>> drivers_;
  kvector<PCIDeviceDriver*> active_drivers_;
};
