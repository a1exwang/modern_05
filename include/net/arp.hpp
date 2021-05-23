#pragma once
#include <cpu_defs.h>
#include <net/ethernet.hpp>
#include <common/kvector.hpp>
#include <common/kspinlock.hpp>
#include <common/klist.hpp>
#include "ipv4.hpp"

constexpr u16 ArpOpcodeRequest = 1;
constexpr u16 ArpOpcodeReply = 2;

constexpr u16 ArpHwAddressSpaceEthernet = 1;
constexpr u16 ArpProtoAddressSpaceIPv4 = 0x0800;

#pragma pack(push, 1)
struct ArpPacket {
  u16 hardware_address_space;
  u16 protocol_address_space;
  u8 hw_len;
  u8 proto_len;
  u16 opcode;
  union {
    u8 data[0];
    struct {
      EthernetAddress hw_sender;
      u32 proto_sender;
      EthernetAddress hw_target;
      u32 proto_target;
    } ethipv4;
  };
};
#pragma pack(pop)

class Arp {
 public:
  explicit Arp(EthernetDriver *driver);
  void HandleRx(EthernetAddress dst, EthernetAddress src, u16 protocol, kvector<u8> data);

  static void ArpThreadStart(void *cookie);
 private:
  void queue_up_mapping(ArpPacket *packet);

  EthernetDriver *driver_;
  u64 arp_kthread_id;

  // this lock locks the following members
  kspinlock lock_;

  kmap<EthernetAddress, IPv4Address> eth2ip;
  kmap<IPv4Address, EthernetAddress> ip2eth;

  kvector<ArpPacket*> packet_queue_;
  std::atomic<size_t> pk_start, pk_end;
  // lock end
};
