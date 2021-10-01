#include <device/gpt.hpp>
#include <common/kvector.hpp>
#include <device/ahci.hpp>
#include <kernel.h>
#include <common/hexdump.hpp>

namespace gpt {

kvector<GPTPrimaryPartition> ListPrimaryPartitions(AHCIDevice &device) {
  // LBA 1 partition table header
  char header_buf[SectorSize];
  const auto result = device.Read(header_buf, SectorSize*1, SectorSize);
  if (result != Error::ErrSuccess) {
    Kernel::k->panic("Failed to read LBA 1");
  }

  if (memcmp(header_buf, "EFI PART", 8) != 0) {
    Kernel::k->panic("EFI magic not match");
  }
  Kernel::sp() << "EFI magic matched!\n";

  auto header = (GPTPartitionTableHeader*)header_buf;
  const auto partition_entries_start = header->PartitionEntryLBA * SectorSize;

  kvector<GPTPrimaryPartition> ret;
  for (int i = 0; i < header->PartitionEntryCount; i++) {
    char partition_entry_buf[SectorSize];
    if (device.Read(
        partition_entry_buf,
        partition_entries_start + header->PartitionEntrySize * i,
        header->PartitionEntrySize) != ErrSuccess) {
      Kernel::k->panic("Failed to read partition entries");
    }

    auto entry = (GPTPartitionEntry*) partition_entry_buf;
    if (entry->PartitionTypeGUID != 0) {
      auto &item = ret.emplace_back();
      memcpy(&item.entry_, entry, header->PartitionEntrySize);
      item.start_offset = item.entry_.FirstLBA * SectorSize;
      item.size = (item.entry_.LastLBA - item.entry_.FirstLBA + 1) * SectorSize;
    }
  }

  return ret;
}

}
