#pragma once
#include <cpu_defs.h>
#include <common/kvector.hpp>
#include <device/ahci.hpp>

namespace gpt {

#pragma pack(push, 1)
struct GPTPartitionTableHeader {
  u64 Signature;
  u32 Revision;
  u32 HeaderSize;
  u32 Crc32;
  u32 Reserved;
  u64 CurrentLBA;
  u64 BackupLBA;
  u64 FirstUsableLBA;
  u64 LastUsableLBA;
  u128 Guid;
  u64 PartitionEntryLBA;
  u32 PartitionEntryCount;
  u32 PartitionEntrySize;
  u32 PartitionEntryCrc32;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct GPTPartitionEntry {
  u128 PartitionTypeGUID;
  u128 PartitionGUID;
  u64 FirstLBA;
  u64 LastLBA;
  u64 AttributeFlags;
  // UTF-16
  wchar_t Name[72];
};
#pragma pack(pop)

struct GPTPrimaryPartition {
  // in bytes
  u64 start_offset;
  u64 size;

  GPTPartitionEntry entry_;
};

kvector<GPTPrimaryPartition> ListPrimaryPartitions(AHCIDevice &device);
}