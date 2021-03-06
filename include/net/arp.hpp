#pragma once
#include <cpu_defs.h>
#include <net/ethernet.hpp>
#include <common/kvector.hpp>
#include <common/kspinlock.hpp>
#include <common/klist.hpp>
#include <net/ipv4.hpp>
#include <optional>
#include <common/kssq.hpp>

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
      u8 proto_sender[4];
      EthernetAddress hw_target;
      u8 proto_target[4];
    } ethipv4;
  };
};
#pragma pack(pop)

class ArpDriver {
 public:
  explicit ArpDriver(EthernetDriver *driver);
  void SetIPDriver(IPDriver *driver) {
    ip_driver_ = driver;
    put(eth_driver_->address(), ip_driver_->address());
  }
  void HandleRx(EthernetAddress dst, EthernetAddress src, u16 protocol, kvector<u8> data);

  std::optional<IPv4Address> find(EthernetAddress mac) const {
    lock_.lock();
    std::optional<IPv4Address> ret;
    auto it = eth2ip.find(mac);
    if (it != eth2ip.end()) {
      ret = it->second;
    }
    lock_.unlock();
    return ret;
  }
  std::optional<EthernetAddress> find(IPv4Address ip) const;
  void put(EthernetAddress eth, IPv4Address ip);

  static void ArpThreadStart(void *cookie);
  void Request(IPv4Address ip);
 private:
  void arp_thread_poll_rx(ArpPacket *&packet);
  void request_gateway_ethernet_address();
  void queue_up_mapping(ArpPacket *packet);
  void handle_request(ArpPacket *packet);

  EthernetDriver *eth_driver_;
  IPDriver *ip_driver_;
  u64 arp_kthread_id;

  // this lock locks the following members
  mutable kspinlock lock_;
  kmap<EthernetAddress, IPv4Address> eth2ip;
  kmap<IPv4Address, EthernetAddress> ip2eth;
  // lock end

  ArpPacket *tmp_pkt = knew<ArpPacket>();
  SSQueue<ArpPacket> rx_queue_ = SSQueue<ArpPacket>(4);
};
