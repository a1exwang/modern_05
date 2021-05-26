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

// ARP thread
//   - consumer of rx_queue and send packet to L2 driver
//   - request gateway MAC periodically
void ArpDriver::ArpThreadStart(void *cookie) {
  auto that = (ArpDriver *)cookie;
  auto packet = knew<ArpPacket>();
  size_t i = 0;
  while (true) {
    // pktqueue consumer
    that->arp_thread_poll_rx(packet);
    kyield();

    // TODO: use timer
    if (i % 10000 == 0) {
      that->request_gateway_ethernet_address();
    }
    i++;
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
    reply_packet.hardware_address_space = cpu2be(ArpHwAddressSpaceEthernet);
    reply_packet.protocol_address_space = cpu2be(ArpProtoAddressSpaceIPv4);
    reply_packet.hw_len = cpu2be((u8)sizeof(EthernetAddress));
    reply_packet.proto_len = cpu2be((u8)sizeof(IPv4Address));
    reply_packet.opcode = cpu2be(ArpOpcodeReply);
    reply_packet.ethipv4.hw_sender = eth_driver_->address();
    memcpy(reply_packet.ethipv4.proto_sender, ip_driver_->address().b, sizeof(IPv4Address));
    reply_packet.ethipv4.hw_target = mac_target.value();
    memcpy(reply_packet.ethipv4.proto_target, &ip_target, sizeof(IPv4Address));
    eth_driver_->TxEnqueue(packet->ethipv4.hw_sender, EtherTypeARP, (u8*)&reply_packet, sizeof(ArpPacket));
  }

}
void ArpDriver::arp_thread_poll_rx(ArpPacket *&packet) {
  // pop to packet with swap and consume the packet
  auto success = rx_queue_.deq(packet);
  if (!success) {
    return;
  }

//    Kernel::sp() << "Got ARP!!!!!!!!!!!!!!!!!!!!!!!!!\n";
//    hexdump((u8*)packet, sizeof(ArpPacket), false, 4);

  put(packet->ethipv4.hw_sender, IPv4Address(packet->ethipv4.proto_sender));

  if (be2cpu(packet->opcode) == ArpOpcodeRequest) {
    handle_request(packet);
  }
}
void ArpDriver::request_gateway_ethernet_address() {
  Request(ip_driver_->gateway_address());
}

void ArpDriver::Request(IPv4Address ip_target) {
  ArpPacket reply_packet;
  reply_packet.hardware_address_space = cpu2be(ArpHwAddressSpaceEthernet);
  reply_packet.protocol_address_space = cpu2be(ArpProtoAddressSpaceIPv4);
  reply_packet.hw_len = cpu2be((u8)sizeof(EthernetAddress));
  reply_packet.proto_len = cpu2be((u8)sizeof(IPv4Address));
  reply_packet.opcode = cpu2be(ArpOpcodeRequest);
  reply_packet.ethipv4.hw_sender = eth_driver_->address();
  memcpy(reply_packet.ethipv4.proto_sender, ip_driver_->address().b, sizeof(IPv4Address));
  memset(&reply_packet.ethipv4.hw_target, 0, sizeof(EthernetAddress));
  memcpy(reply_packet.ethipv4.proto_target, &ip_target, sizeof(IPv4Address));
  eth_driver_->TxEnqueue(EthernetAddress::Broadcast(), EtherTypeARP, (u8*)&reply_packet, sizeof(ArpPacket));
}
std::optional<EthernetAddress> ArpDriver::find(IPv4Address ip) const {
  lock_.lock();
  std::optional<EthernetAddress> ret;
  auto it = ip2eth.find(ip);
  if (it != ip2eth.end()) {
    ret = it->second;
  }
  lock_.unlock();
  return ret;
}
void ArpDriver::put(EthernetAddress eth, IPv4Address ip) {
  lock_.lock();
  eth2ip[eth] = ip;
  ip2eth[ip] = eth;
  lock_.unlock();
}
