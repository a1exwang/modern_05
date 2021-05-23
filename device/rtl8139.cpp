#include <device/rtl8139.hpp>
#include <cpu_utils.h>
#include <kernel.h>
#include <mm/page_alloc.h>
#include <cpu_defs.h>
#include <lib/string.h>
#include <device/pci.h>
#include <irq.hpp>
#include <mm/mm.h>
#include <net/ethernet.hpp>
#include <common/hexdump.hpp>
#include <net/arp.hpp>
#include <common/endian.hpp>
#include <process.h>
#include <common/kssq.hpp>

constexpr u32 RxOk = 1 << 0;
constexpr u32 RxError = 1 << 1;
constexpr u32 TxOk = 1 << 2;
constexpr u32 TxError = 1 << 3;
constexpr u32 Timeout = 1 << 14;
constexpr u32 RxOverflow = 0x10;
constexpr u32 RxFifoOverflow = 0x40;
constexpr u32 RxAck = RxOk | RxOverflow | RxFifoOverflow;


#pragma pack(push, 1)
struct Rtl8139Register {
  u8 mac[6];
  u8 unuseda[2];
  u8 mar[8];

  // 0x10
  u32 tx_status[4];
  u32 tx_start_addr[4];

  // 0x30
  u32 rx_buffer_start_addr;

  u8 unused2[2];
  u8 early_rx_status;
  u8 cmd;
  u16 current_address_of_packet_read;
  u16 current_buffer_address;
  u16 imr;
  u16 isr;

  // 0x40
  u32 tx_config;
  u32 rx_config;
  u8 unused42[8];

  // 0x50
  u8 unused5[2];
  u8 config1;
};
#pragma pack(pop)

constexpr size_t TxQueueSize = 32;

class Rtl8139Device {
 public:
  static kvector<KEthernetPacket*> init_tx_queue() {
    kvector<KEthernetPacket*> ret(TxQueueSize);
    for (auto &item : ret) {
      item = create_packet();
    }
    return ret;
  }
  Rtl8139Device(volatile ExtendedConfigSpace *pci_config_space, volatile void *regs_base)
      :
      config_space(pci_config_space),
      regs(reinterpret_cast<volatile Rtl8139Register*>(regs_base)),
      rx_buffer_size(1<<16),
      tx_buffer_size(1<<16),
      tx_queue_(init_tx_queue()) {

    rx_buffer = (volatile char*) kernel_page_alloc(16);
    tx_buffer_raw = (char*) kernel_page_alloc(16);
    tx_buffer_raw_phy = (u32)(u64)(tx_buffer_raw - KERNEL_START);
    for (int i = 0; i < 4; i++) {
      tx_buffer[i] = tx_buffer_raw + 4096*i;
      tx_buffer_phy[i] = tx_buffer_raw_phy + 4096*i;
    }

    // pci enable bus mastering
    config_space->command = config_space->command | (1<<2);
    // enable IRQ
    config_space->command = config_space->command & ~(1<<10);

    // use legacy IO APIC to manage IRQ

    // pci init unable to use MSI MSI-X
//    // has capabilites list
//    if (config_space->status & (1 << 4)) {
//      u8 *cap_entry = ((u8*)config_space + (config_space->capabilities_pointer & 0xfc));
//      while (true) {
//        auto next_ptr = cap_entry[1] & 0xfc;
//        if (next_ptr == 0) {
//          break;
//        }
//        if (cap_entry[0] == 0x5) {
//          auto message_control = *(u16*)(cap_entry+2);
//          auto message_address = *(u64*)(cap_entry+4);
//          auto message_data = *(u16*)(cap_entry+0xc);
//          auto mask = *(u32*)(cap_entry+0x10);
//          auto pending = *(u32*)(cap_entry+0x14);
//          Kernel::sp() << "support MSI\n";
//        }
//
//        cap_entry += next_ptr;
//      }
//
//
//    }

  }

  void SetArp(ArpDriver *arp) {
    arp_ = arp;
  }

  static void KthreadEntry(void *cookie) {
    ((Rtl8139Device*)cookie)->kthread_entry();
  }

  void kthread_entry();

  void start_kthread();

  void reset() {

    // power on
    regs->config1 = 0;

    // software reset
    regs->cmd = 0x10;

    // waiting for reset
    Kernel::sp() << "starting to reset rtl8139\n";
    while ((regs->cmd & 0x10) != 0);
    Kernel::sp() << "reset rtl8139 done\n";

    // print MAC
    Kernel::sp() << "MAC Address = ";
    memcpy(this->mac.data, (const void*)regs->mac, sizeof(this->mac.data));
    hexdump((const char*)regs->mac, 6, true);
    Kernel::sp() << "\n";

    // setup RX buffer
    u32 rx_buffer_phy = (u32)(u64)(rx_buffer - KERNEL_START);
    regs->rx_buffer_start_addr = rx_buffer_phy;

    // config rx buffer
    // accept all packets, physical match packets, multicast, broadcast
    // enable overflow (we are safe because we have 64k buffer, far greater than 8k+16)
    // 8192+16 buffer size
    regs->rx_config = 0xf | (1 << 7);

    // Interframe gap time = 960ns
    // DMA burst = 2048
    // retry count = 8
    regs->tx_config = (3 << 24) | (7 << 8) | (8 << 4);
    for (int i = 0; i < 4; i++) {
      regs->tx_start_addr[i] = tx_buffer_phy[i];
    }

    // enable rx/tx
    regs->cmd = 0xc;

    // enable IRQ mask: all
    regs->imr = 0xe07f;
  }

  void test() {
//    if (!tx_async(test_message, 42)) {
//      Kernel::sp() << "tx fail\n";
//    }
  }

  void RxUpper(EthernetAddress dst, EthernetAddress src, u16 protocol, kvector<u8> data) {
//    Kernel::sp() << "  L3 payload: dst ";
//    dst.print();
//    Kernel::sp() << " src ";
//    src.print();
//    Kernel::sp() << " protocol 0x" << IntRadix::Hex << protocol << "\n";
//    hexdump(data.data(), data.size(), false, 4);

    if (protocol == EtherTypeARP) {
      assert1(arp_ != nullptr);
      arp_->HandleRx(dst, src, protocol, std::move(data));
    }
    // TODO: handle other packet types
  }

  void consume_rx(size_t packet_size) {
    // align 4byte
    rx_offset = (rx_offset + packet_size + 4 + 3) & ~3;
    regs->current_address_of_packet_read = rx_offset - 0x10;
    rx_offset %= (8192);
  }

  void handle_rx() {
    auto status = *(u32*)(rx_buffer + rx_offset);
    auto packet_size = (status >> 16);
    if (packet_size == 0) {
      Kernel::sp() << "RTL8139 rx 0\n";
      return;
    }

    // drop packet crc32 not matched
    const char *data = (const char*)rx_buffer + rx_offset + 4;
    auto calculated_crc32 = crc32iso_hdlc(0, data, packet_size-4);
    auto stored_crc32 = *(u32*)&data[packet_size-4];
    if (stored_crc32 != calculated_crc32) {
      Kernel::sp() << "CRC32 not match, drop packet! 0x" << IntRadix::Hex << stored_crc32 << " != 0x" << calculated_crc32 << "\n";
      consume_rx(packet_size);
      return;
    }

    Kernel::sp() << "RTL8139 rx 0x" << rx_offset << " 0x" << packet_size << " capr 0x" << regs->current_address_of_packet_read << " cbr " << regs->current_buffer_address << "\n";
//    hexdump(data, packet_size, false, 4);
//    Kernel::sp() << "\n";

    // skip dst, src and protocol
    const u8 *upper_data = (u8*)data + 6 + 6 + 2;
    size_t upper_size = packet_size - 6 - 6 - 2 - 4; // subtract checksum at the end
    kvector<u8> upper_buffer(upper_size);
    memcpy(upper_buffer.data(), upper_data, upper_size);
    EthernetAddress dst, src;
    memcpy(dst.data, data, sizeof(dst.data));
    memcpy(src.data, data+sizeof(src.data), sizeof(dst.data));
    u16 protocol = be(*(u16*)(data+sizeof(EthernetAddress)*2));

    // TODO: support multicast
    // drop non-unicast packet
    if (!dst.IsBroadcast() && dst != mac) {
      consume_rx(packet_size);
      return;
    }

    RxUpper(dst, src, protocol, std::move(upper_buffer));
  }

  void handle_tx() {
    tx_buffer_index++;
    tx_buffer_index %= 4;
    tx_done_ = true;
    Kernel::sp() << "RTL8139 tx ok\n";
  }

  bool irq() {
    if (regs->isr == 0) {
      return false;
    }
    // clear RX OK
    if (regs->isr & RxOk) {
      handle_rx();
      regs->isr = RxOk;
    }
    if (regs->isr & RxError) {
      Kernel::sp() << "RTL8139 rx error\n";
      regs->isr = RxError;
    }
    if (regs->isr & TxOk) {
      handle_tx();
      regs->isr = TxOk;
    }
    if (regs->isr & TxError) {
      Kernel::sp() << "RTL8139 tx error\n";
      regs->isr = TxError;
    }
    if (regs->isr & RxOverflow) {
      Kernel::sp() << "RTL8139 rx buffer overflow\n";
    }
    if (regs->isr & (1<<5)) {
      Kernel::sp() << "RTL8139 packet underrun/link change\n";
    }
    if (regs->isr & RxFifoOverflow) {
      Kernel::sp() << "RTL8139 rx fifo overflow\n";
    }
    if (regs->isr & (1<<13)) {
      Kernel::sp() << "RTL8139 cable length changed\n";
    }
    if (regs->isr & Timeout) {
      Kernel::sp() << "RTL8139 timeout\n";
    }
    if (regs->isr & (1<<15)) {
      Kernel::sp() << "RTL8139 system error\n";
    }
    // NOTE: clear isr, must use 1 to clear this
    regs->isr = RxAck;
    return true;
  }

//  void SendPacket(EthernetAddress src, EthernetAddress dst, std::span<const u8> data) {
//
//  }
//
//  void ReceivePacket(EthernetAddress src, EthernetAddress dst, std::span<u8> data) {
//
//  }

  unsigned long crc32iso_hdlc(unsigned long crc, void const *mem, size_t len) {
    auto data = (unsigned char const *) mem;
    if (data == NULL)
      return 0;
    crc ^= 0xffffffff;
    while (len--) {
      crc ^= *data++;
      for (unsigned k = 0; k < 8; k++)
        crc = crc & 1 ? (crc >> 1) ^ 0xedb88320 : crc >> 1;
    }
    return crc ^ 0xffffffff;
  }

  bool tx_ready() const {
    return regs->tx_status[tx_buffer_index] & (1<<13);
  }

  bool TxEnqueue(KEthernetPacket* &packet) {
    return tx_queue_.enq(packet);
  }

 public:
  EthernetAddress mac;

 private:
  bool tx_async(const void *buffer, u64 size);

 private:
  volatile char *rx_buffer;
  u64 rx_buffer_size;
  u32 rx_offset = 0;

  char *tx_buffer_raw;
  u32 tx_buffer_raw_phy;
  char *tx_buffer[4];
  u32 tx_buffer_phy[4];
  u64 tx_buffer_size;
  int tx_buffer_index = 0;
  std::atomic<bool> tx_done_ = false;

  volatile Rtl8139Register *regs;
  volatile ExtendedConfigSpace *config_space;

  ArpDriver *arp_;
  SSQueue<KEthernetPacket> tx_queue_;
};
void Rtl8139Device::start_kthread() {
  create_kthread("rtl8139", Rtl8139Device::KthreadEntry, this);
}

void Rtl8139Device::kthread_entry() {
  auto packet = create_packet();
  size_t i = 0;
  while (true) {
    auto success = tx_queue_.deq(packet);
    if (!success) {
      i++;
      kyield();
      continue;
    }

    while (!tx_ready()) {
      kyield();
    }

    size_t size = EthernetHeaderSize + packet->size;
    bool queued = tx_async(packet->pkt_start, size);
    if (!queued) {
      Kernel::sp() << "rtl8139 drop tx packet, nic tx reentry\n";
      continue;
    }

    // wait for NIC to ack the tx via IRQ
    // TODO: replace this busy wait
    while (!tx_done_);

    tx_done_ = false;
  }
}
bool Rtl8139Device::tx_async(const void *buffer, unsigned long size) {
  if (!tx_ready()) {
    return false;
  }

  // max size 1792
  auto tx_buffer_phy = (u64)buffer - KERNEL_START;
  assert(tx_buffer_phy < (1ul<<32), "invalid tx buffer set, must be < 4G");

  // expand to 64-4 bytes if shorter
  memcpy(tx_buffer[tx_buffer_index], buffer, size);
  if (size < 64 - 4) {
    memset(&tx_buffer[tx_buffer_index] + size, 0, 64-4-size);
    size = 64 - 4;
  }
  u32 crc32 = crc32iso_hdlc(0, tx_buffer[tx_buffer_index], size);
  // append crc32
  *(u32*)&tx_buffer[tx_buffer_index][size] = le(crc32);
  size += 4;

  // set size and start tx
  regs->tx_status[tx_buffer_index] =
      (regs->tx_status[tx_buffer_index] & ~(1<<13)) | (size & 0xfff);

  return true;
}

static Rtl8139Device *dev;

void write_ioapic_register(void *apic_base, const u8 offset, const u32 val) {
  /* tell IOREGSEL where we want to write to */
  *(volatile u32*)(apic_base) = offset;
  /* write the value to IOWIN */
  *(volatile u32*)((char*)apic_base + 0x10) = val;
}

u32 read_ioapic_register(void *apic_base, const u8 offset) {
  /* tell IOREGSEL where we want to read from */
  *(volatile u32*)(apic_base) = offset;
  /* return the data from IOWIN */
  return *(volatile u32*)((char*)apic_base + 0x10);
}

void ioapic_init(SystemDescriptionTable* table) {
  // https://wiki.osdev.org/MADT
  auto flags = *(u32*)(table->data + 4);
  if (flags & 1) {
    // need to disable 8259 PIC
    outb(0x21, 0xff);
    outb(0xa1, 0xff);
    Kernel::sp() << "disabling 8259 PIC\n";
  }

  u32 ioapic0_phy_addr = 0;
  u32 ioapic0_gsb = 0;

  auto rest = table->length - sizeof(SystemDescriptionTable) - 8;
  u8 *p = table->data + 8;
  u64 offset = 0;
  while (offset < rest) {
    auto entry_type = p[offset];
    auto len = p[offset+1];

    // IO APIC
    if (entry_type == 1) {
      auto id = p[offset+2];
      auto phy_addr = *(u32*)(p+offset+4);
      auto global_system_interrupt_base = *(u32*)(p+offset+8);
      Kernel::sp() << "IO APIC 0x" << IntRadix::Hex << id << " 0x" << phy_addr << " 0x" << global_system_interrupt_base << "\n";
      ioapic0_phy_addr = phy_addr;
      ioapic0_gsb = global_system_interrupt_base;
      break;
    }
    offset += len;
  }

  assert(ioapic0_phy_addr != 0, "No IO APIC available!");

  auto ioapic0 = (void*)(KERNEL_START + ioapic0_phy_addr);
  // IOAPICVER
  u32 v = read_ioapic_register(ioapic0, 0x1);
  auto max_redir_entry = ((v >> 16) & 0xff) + 1;
  Kernel::sp() << "IOAPIC0 version 0x" << IntRadix::Hex << (v & 0xff) << " 0x" << max_redir_entry << "\n";

  assert(max_redir_entry >= 1, "NO IRQ avaialble for this IO APIC");

  u32 irq = 0x40;
  for (int i = 0; i < max_redir_entry; i++) {
    // deliver mode: fixed
    // destination mode: physical
    // pin polarity active low
    // trigger: edge
    // mask: false
    u32 target_lapic_id = 0;
    write_ioapic_register(ioapic0, 0x10 + 2*i, (irq+i) | (1<<13));
    write_ioapic_register(ioapic0, 0x11 + 2*i, (target_lapic_id << 24));

    auto lo = read_ioapic_register(ioapic0, 0x10 + 2*i);
    auto hi = read_ioapic_register(ioapic0, 0x11 + 2*i);
    Kernel::sp() << "    IOAPIC " << i << ", " << lo << " " << hi << "\n";
  }
}

void lapic_eoi();

void rtl8139_test() {
  dev->test();
}

ArpDriver *arp1;

bool RTL8139Driver::Enumerate(PCIDeviceInfo *info) {
  bool found = false;
  PCIBar b;
  for (auto bar : info->bars) {
    if (!bar.is_io) {
      b = bar;
      found = true;
      break;
    }
  }

  assert1(found);
  volatile void *regs_base = phy2virt(b.start);
  dev = knew<Rtl8139Device>(info->config_space, regs_base);
  // TODO: unify management of this objects
  auto arp = knew<ArpDriver>(this);
  arp1 = arp;

  auto ip = knew<IPDriver>(this, arp);
  arp->SetIPDriver(ip);

  dev->SetArp(arp);
  SetIPDriver(ip);
  dev->reset();

  dev->start_kthread();

  return true;
}
bool RTL8139Driver::HandleInterrupt(unsigned long irq_num) {
  bool handled = dev->irq();
  if (handled) {
    lapic_eoi();
  }
  return handled;
}

bool RTL8139Driver::TxEnqueue(EthernetAddress dst, u16 protocol, u8 *payload, size_t size) {
  kvector<u8> buf(sizeof(dst.data)*2+sizeof(u16)+size);
  auto packet = create_packet();
  packet->size = size;
  packet->dst = dst;
  packet->src = dev->mac;
  packet->protocol = cpu2be(protocol);
  memcpy(packet->data, payload, size);
  return dev->TxEnqueue(packet);
}
EthernetAddress RTL8139Driver::address() const {
  return dev->mac;
}
