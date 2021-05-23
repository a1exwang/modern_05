#include <net/arp.hpp>
#include <net/ethernet.hpp>
#include <process.h>
#include <common/endian.hpp>

void Arp::HandleRx(EthernetAddress dst, EthernetAddress src, u16 protocol, kvector<u8> data) {
  auto packet = reinterpret_cast<ArpPacket*>(data.data());
  // TODO: support other hardware address and protocol address
  if (!(be(packet->hardware_address_space) == ArpHwAddressSpaceEthernet && be(packet->protocol_address_space) == ArpProtoAddressSpaceIPv4)) {
    Kernel::sp() << "ARP received with unknown hw addr or proto addr\n";
    return;
  }

  assert1(packet->hw_len == 6);
  assert1(packet->proto_len == 4);

  if (be(packet->opcode) == ArpOpcodeRequest) {
    queue_up_mapping(packet);
    // TODO: reply them with respect
  } else if (be(packet->opcode) == ArpOpcodeReply) {
    queue_up_mapping(packet);
  } else {
    Kernel::sp() << "ARP received with unknown opcode 0x" << IntRadix::Hex << packet->opcode << "\n";
    return;
  }
}
Arp::Arp(EthernetDriver *driver) :driver_(driver), packet_queue_(4), pk_start(0), pk_end(0) {
  arp_kthread_id = create_kthread("arp", Arp::ArpThreadStart, this);
  for (auto &item : packet_queue_) {
    item = knew<ArpPacket>();
  }
}

void Arp::ArpThreadStart(void *cookie) {
  auto that = (Arp *)cookie;
  auto packet = knew<ArpPacket>();
  // pk_start is owned by us and it only goes forward
  while (true) {
    // pktqueue consumer
    if (that->pk_start == that->pk_end) {
      // empty queue
//      Kernel::sp() << "empty queue\n";
      kyield();
      continue;
    }

    std::swap<ArpPacket*>(that->packet_queue_[that->pk_start], packet);
    that->pk_start = (that->pk_start + 1) % that->packet_queue_.size();

    Kernel::sp() << "Got ARP!!!!!!!!!!!!!!!!!!!!!!!!!\n";
    // TODO: handle packet in packet
    hexdump((u8*)packet, sizeof(ArpPacket), false, 4);

    kyield();
  }
}

void Arp::queue_up_mapping(ArpPacket *packet) {
  // pk_end is owned by us and it only goes forward
  auto end = pk_end.load();
  auto next = (end + 1) % packet_queue_.size();
  if (next == pk_start) {
    // queue full, drop new packet
    return;
  }

  auto pkt = packet_queue_[end];
  memcpy(pkt, packet, sizeof(*pkt));

  pk_end = next;
  Kernel::sp() << "queue mapping " << next << "\n";
}
