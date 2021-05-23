#include <net/arp.hpp>
#include <net/ethernet.hpp>
#include <process.h>
#include <common/endian.hpp>

void ArpDriver::HandleRx(EthernetAddress dst, EthernetAddress src, u16 protocol, kvector<u8> data) {
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
ArpDriver::ArpDriver(EthernetDriver *driver) : eth_driver_(driver) {
  arp_kthread_id = create_kthread("arp", ArpDriver::ArpThreadStart, this);
}

// ARP thread, consumer of rx pk_queue
void ArpDriver::ArpThreadStart(void *cookie) {
  auto that = (ArpDriver *)cookie;
  auto packet = knew<ArpPacket>();
  // pk_start is owned by us and it only goes forward
  while (true) {
    // pktqueue consumer

    // pop to packet with swap and consume the packet
    auto success = that->rx_queue_.deq(packet);
    if (!success) {
      kyield();
      continue;
    }

    Kernel::sp() << "Got ARP!!!!!!!!!!!!!!!!!!!!!!!!!\n";
    hexdump((u8*)packet, sizeof(ArpPacket), false, 4);

    that->put(packet->ethipv4.hw_sender, IPv4Address(be(packet->ethipv4.proto_sender)));

    if (be(packet->opcode) == ArpOpcodeRequest) {
      that->handle_request(packet);
    }

    kyield();
  }
}

void ArpDriver::queue_up_mapping(ArpPacket *packet) {
  memcpy(tmp_pkt, packet, sizeof(ArpPacket));

  auto success = rx_queue_.enq(tmp_pkt);
  if (!success) {
    Kernel::sp() << "Warning, ARP rx queue full, dropping new pkt\n";
  }
}

void ArpDriver::handle_request(ArpPacket *packet) {
  assert1(be(packet->opcode) == ArpOpcodeRequest);
  auto ip_target = packet->ethipv4.proto_target;
  auto mac_target = find(ip_target);
  if (mac_target.has_value() && ip_driver_) {
    ArpPacket reply_packet;
    reply_packet.hardware_address_space = ArpHwAddressSpaceEthernet;
    reply_packet.protocol_address_space = ArpProtoAddressSpaceIPv4;
    reply_packet.hw_len = sizeof(EthernetAddress);
    reply_packet.proto_len = sizeof(IPv4Address);
    reply_packet.opcode = ArpOpcodeReply;
    reply_packet.ethipv4.hw_sender = eth_driver_->address();
    reply_packet.ethipv4.proto_sender = ip_driver_->address().network_order();
    reply_packet.ethipv4.hw_target = mac_target.value();
    reply_packet.ethipv4.proto_target = ip_target;
//    eth_driver_->Tx(packet->ethipv4.hw_sender, ArpOpcodeReply, (u8*)&reply_packet, sizeof(ArpPacket));
  }

}
