#include <cpu_utils.h>
#include <kernel.h>
#include <mm/page_alloc.h>
#include <cpu_defs.h>
#include <lib/string.h>
#include <device/pci.h>

constexpr u32 RxOk = 0x01;
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

  u8 unused2[3];
  u8 cmd;
  u16 current_address_of_packet_read;
  u16 current_buffer_address;
  u16 imr;
  u16 isr;

  // 0x40
  u8 unused4[4];
  u32 receive_configuration_register;
  u8 unused42[8];

  // 0x50
  u8 unused5[2];
  u8 config1;
};
#pragma pack(pop)

const char ping_172_20_0_1[] =
    "\xea\xa3\x3e\x38\xca\x4d\x52\x54\x00\x12\x34\x56\x08\x00\x45\x00" \
    "\x00\x54\x7b\x1e\x40\x00\x40\x01\x67\x5e\xac\x14\x00\x03\xac\x14" \
    "\x00\x01\x08\x00\x39\x11\x00\x01\x00\x01\x83\x91\x9a\x60\x00\x00" \
    "\x00\x00\xe1\x27\x01\x00\x00\x00\x00\x00\x10\x11\x12\x13\x14\x15" \
    "\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f\x20\x21\x22\x23\x24\x25" \
    "\x26\x27\x28\x29\x2a\x2b\x2c\x2d\x2e\x2f\x30\x31\x32\x33\x34\x35" \
    "\x36\x37";


class Rtl8139Device {
 public:
  Rtl8139Device(volatile ExtendedConfigSpace *pci_config_space, volatile void *io_base)
      :
      config_space(pci_config_space),
      regs(reinterpret_cast<volatile Rtl8139Register*>(io_base)),
      rx_buffer_size(1<<16),
      tx_buffer_size(1<<16) {

    rx_buffer = (volatile char*) kernel_page_alloc(16);
    tx_buffer = (char*) kernel_page_alloc(16);

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
    hexdump((const char*)regs->mac, 6, true);
    Kernel::sp() << "\n";

    // setup RX buffer
    u32 rx_buffer_phy = (u32)(u64)(rx_buffer - KERNEL_START);
    regs->rx_buffer_start_addr = rx_buffer_phy;

    // setup IRQ mask: Receive OK + Transmit OK
    regs->imr = 0xe07f;

    // config rx buffer
    // accept all packets, physical match packets, multicast, broadcast
    // enable overflow (we are safe because we have 64k buffer, far greater than 8k+16)
    // 8192+16 buffer size
    regs->receive_configuration_register = 0xf | (1<<7);

    // enable rx/tx
    regs->cmd = 0xc;

    // try ping
//    tx_sync(ping_172_20_0_1, sizeof(ping_172_20_0_1));
//
//    Kernel::sp() << "tx done\n";
//
//    while(1);
  }

  void test() {
    tx_sync(ping_172_20_0_1, sizeof(ping_172_20_0_1));
    Kernel::sp() << "tx done\n";
  }

  void handle_rx() {
    auto status = *(u32*)(rx_buffer + rx_offset);
    auto packet_size = (status >> 16);
    if (packet_size == 0) {
      Kernel::sp() << "RTL8139 rx 0\n";
      return;
    }
    Kernel::sp() << "RTL8139 rx 0x" << rx_offset << " 0x" << packet_size << " capr 0x" << regs->current_address_of_packet_read << " cbr " << regs->current_buffer_address << "\n";
    hexdump((const char*)(rx_buffer + rx_offset + 4), packet_size, false, 4);
    Kernel::sp() << "\n";
    rx_offset = (rx_offset + packet_size + 4 + 3) & ~3;
    regs->current_address_of_packet_read = rx_offset - 0x10;
    rx_offset %= (8192);
  }

  void handle_tx() {
    Kernel::sp() << "RTL8139 tx ok\n";
  }

  void irq() {
    // clear RX OK
    if (regs->isr != 0) {
      if (regs->isr & 0x1) {
        handle_rx();
      }
      if (regs->isr & 0x2) {
        Kernel::sp() << "RTL8139 rx error\n";
      }
      if (regs->isr & 0x4) {
        handle_tx();
      }
      if (regs->isr & 0x8) {
        Kernel::sp() << "RTL8139 tx error\n";
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
      if (regs->isr & (1<<14)) {
        Kernel::sp() << "RTL8139 timeout\n";
      }
      if (regs->isr & (1<<15)) {
        Kernel::sp() << "RTL8139 system error\n";
      }
      // NOTE: clear isr, must use 1 to clear this
      regs->isr = RxAck;
    }
  }

  void tx_sync(const void *buffer, u64 size) {
    // max size 1792
    u32 tx_buffer_phy = (u64)buffer - KERNEL_START;
    assert(tx_buffer_phy < (1ul<<32), "invalid tx buffer set, must be < 4G");
    regs->tx_start_addr[tx_buffer_index] = tx_buffer_phy;

    memcpy(tx_buffer, ping_172_20_0_1, sizeof(ping_172_20_0_1));

    // set size
    regs->tx_status[tx_buffer_index] = regs->tx_status[tx_buffer_index] | (size & 0xfff);

    // start tx
    regs->tx_status[tx_buffer_index] =
        regs->tx_status[tx_buffer_index] & (1u<<13);

    while (regs->tx_status[tx_buffer_index] & (1<<15));
    tx_buffer_index++;
    // set own bit (should be unnecessary)
    regs->tx_status[tx_buffer_index] =
        regs->tx_status[tx_buffer_index] | (1u<<13);
  }

 private:
  volatile char *rx_buffer;
  u64 rx_buffer_size;
  char *tx_buffer;
  u64 tx_buffer_size;

  u32 rx_offset = 0;

  int tx_buffer_index = 0;

  volatile Rtl8139Register *regs;
  volatile ExtendedConfigSpace *config_space;
};

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

  assert(max_redir_entry >= 1, "NO IRQ avaialble for this IO APIC")

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

void rtl8139_init(volatile ExtendedConfigSpace* ecs, volatile void *io_base) {

  void *buf = kernel_page_alloc(16);
  dev = new(buf) Rtl8139Device(ecs, io_base);
  dev->reset();
}

void rtl8139_test() {
  dev->test();
}

void lapic_eoi();
void handle_pci_irqs() {
  lapic_eoi();
  dev->irq();
}
