#pragma once

namespace fs::fat32 {

#pragma pack(push, 1)
struct Bpb {
  char jumpInstruction[3];
  char oemIdentifier[8];
  u16 bytesPerSector;
  u8 sectorsPerCluster;
  u16 reservedSectorCount;
  u8 fatCount;
  u16 dirEntryCount;
  u16 sectorCount; // 0 for values > 65535
  u8 mediaDescriptorType;
  u16 reserved1;
  u16 sectorsPerTrack
  u16 headCount;
  u32 hiddenSectorCount;
  u32 largeSectorCount;

  union {
    EbrFat32 ebrFat32;
  };
  // extended boot record for
};

struct EbrFat32 {
  u32 sectorsPerFat;
  u16 flags;
  u16 version;
  u32 clusterNumberOfRootDir;
  u16 sectorNumberFSInfo;
  u16 sectorNumberBackupBootSector;
  char reserved1[12];
  u8 driverNumber;
  u8 ntFlags;
  u8 signature; // 0x28 or 0x29
  u32 volumeIdSerial;
  char label[11];
  char systemIdentifier[8];
  char bootCode[420];
  u16 bootablePartitionSignature; // 0xAA55
};

struct FSInfo {
  u32 signature; // 0x41615252
  char reserved[480];
  u32 signature2; // 0x61417272
  u32 lastKnownFreeClusterCount;
  u32 hintStartCluster;
  char reserved1[12];
  u32 signature3; // 0xAA550000
};

#pragma pack(pop)

}
