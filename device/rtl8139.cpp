#include <cpu_utils.h>
#include <kernel.h>
#include <mm/page_alloc.h>
#include <cpu_defs.h>
#include <lib/string.h>
#include <device/pci.h>
#include <irq.hpp>

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

const char test_message[] =
    "\xff\xff\xff\xff\xff\xff\x0a\x01\x0e\x0a\x01\x0e\x08\x06\x00\x01" \
    "\x08\x00\x06\x04\x00\x01\x0a\x01\x0e\x0a\x01\x0e\xac\x14\x00\x03" \
    "\x00\x00\x00\x00\x00\x00\xac\x14\x00\x01";



class Rtl8139Device {
 public:
  Rtl8139Device(volatile ExtendedConfigSpace *pci_config_space, volatile void *io_base)
      :
      config_space(pci_config_space),
      regs(reinterpret_cast<volatile Rtl8139Register*>(io_base)),
      rx_buffer_size(1<<16),
      tx_buffer_size(1<<16) {

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

    tx_available = true;

    // enable IRQ mask: all
    regs->imr = 0xe07f;
  }

  void test() {
    if (!tx_async(test_message, 42)) {
      Kernel::sp() << "tx fail\n";
    }
  }

  void handle_rx() {
    auto status = *(u32*)(rx_buffer + rx_offset);
    auto packet_size = (status >> 16);
    if (packet_size == 0) {
      Kernel::sp() << "RTL8139 rx 0\n";
      return;
    }
    Kernel::sp() << "RTL8139 rx 0x" << rx_offset << " 0x" << packet_size << " capr 0x" << regs->current_address_of_packet_read << " cbr " << regs->current_buffer_address << "\n";
    const char *data = (const char*)rx_buffer + rx_offset + 4;
    hexdump(data, packet_size, false, 4);
    auto calculated_crc32 = crc32iso_hdlc(0, data, packet_size-4);
    auto stored_crc32 = *(u32*)&data[packet_size-4];
    if (stored_crc32 != calculated_crc32) {
      Kernel::sp() << "CRC32 not match! 0x" << IntRadix::Hex << stored_crc32 << " != 0x" << calculated_crc32 << "\n";
    }
    Kernel::sp() << "\n";
    rx_offset = (rx_offset + packet_size + 4 + 3) & ~3;
    regs->current_address_of_packet_read = rx_offset - 0x10;
    rx_offset %= (8192);
  }

  void handle_tx() {
    tx_available = true;
    tx_buffer_index++;
    tx_buffer_index %= 4;
    Kernel::sp() << "RTL8139 tx ok\n";
  }

  void irq() {
    // clear RX OK
    if (regs->isr != 0) {
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
    }
  }

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

  u32 swap_endian(u32 v) {
    return ((v>>0) & 0xff) << 24 |
        ((v>>8) & 0xff) << 16 |
        ((v>>16) & 0xff) << 8 |
        ((v>>24) & 0xff) << 0;
  }

  u32 be(u32 v) {
    return swap_endian(v);
  }
  u32 le(u32 v) {
    return v;
  }

  bool tx_async(const void *buffer, u64 size) {
    if (!tx_available) {
      return false;
    }
    tx_available = false;

    // wait for host ready
    while (!(regs->tx_status[tx_buffer_index] & (1<<13)));

    // max size 1792
    u64 tx_buffer_phy = (u64)buffer - KERNEL_START;
    assert(tx_buffer_phy < (1ul<<32), "invalid tx buffer set, must be < 4G");

    // expand to 64-4 bytes if shorter
    memcpy(tx_buffer[tx_buffer_index], buffer, size);
    if (size < 64 - 4) {
      memset(&tx_buffer[tx_buffer_index] + size, 0, 64-4-size);
      size = 64 - 4;
    }
    auto crc32 = crc32iso_hdlc(0, tx_buffer[tx_buffer_index], size);
    // append crc32
    *(u32*)&tx_buffer[tx_buffer_index][size] = le(crc32);
    size += 4;

    // set size and start tx
    regs->tx_status[tx_buffer_index] =
        (regs->tx_status[tx_buffer_index] & ~(1<<13)) | (size & 0xfff);

    return true;
  }

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
  bool tx_available = false;

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

void rtl8139_init(volatile ExtendedConfigSpace* ecs, volatile void *io_base) {
  void *buf = kernel_page_alloc(16);
  dev = new(buf) Rtl8139Device(ecs, io_base);
  dev->reset();

  Kernel::k->irq_->Register(0x40, 0x18, [](u64 irq_num, u64 error_num, Context *context) {
    dev->irq();
    lapic_eoi();
  });
}

void rtl8139_test() {
  dev->test();
}

