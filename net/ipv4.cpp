#include <net/ipv4.hpp>
#include <net/arp.hpp>

void IPDriver::HandleRx(EthernetAddress dst, EthernetAddress src, u16 protocol, kvector<u8> data) {
  auto src_ip = arp_driver_->find(src);
  if (!src_ip) {
    Kernel::sp() << "Unknown sender MAC ";
    src.print();
    Kernel::sp() << "\n";
  } else {
    Kernel::sp() << "IP rx from ";
    src_ip.value().print();
    Kernel::sp() << "\n";
  }
}
