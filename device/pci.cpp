#include <cpu_defs.h>
#include <lib/port_io.h>
#include <kernel.h>
#include <device/pci.h>
#include <lib/string.h>
#include <mm/page_alloc.h>
#include <irq.hpp>

u16 pci_config_read_word(u16 bus, u16 slot, u16 func, u16 offset){
  unsigned short tmp = 0;

  /* create configuration address as per Figure 1 */
  u32 address = (u32)(((u32)bus << 16) | ((u32)slot << 11) |
      ((u32)func << 8) | ((u32)offset & 0xfc) | ((u32)0x80000000));

  outl(0xCF8, address);

  /* (offset & 2) * 8) = 0 will choose the fisrt word of the 32 bits register */
  return (unsigned short)((inl(0xCFC) >> ((offset & 2) * 8)) & 0xffff);
}

// returns <start, size, is_io_addr>
std::tuple<u32, u32, bool> bar_parse(volatile u32 *bar) {
  u32 old = *bar;
  if (old & 0x1) {
    // IO space bar
    *bar = 0xfffffffcu | (old & 0x3);
    u32 size = ~((*bar & 0xfffffffcu)) + 1;
    *bar = old;
    return {old & 0xfffffffcu, size, true};
  } else {
    // memory space bar;
    *bar = 0xfffffff0u | (old & 0xf);
    u32 size = ~((*bar & 0xfffffff0u)) + 1;
    *bar = old;
    return {old & 0xffffffffu, size, false};
  }
}

void lapic_eoi();
void PCIBusDriver::Enumerate(ECMGroup *segment_groups, size_t n) {
  Kernel::sp() << "Enumerating PCI devices:\n";
  for (u64 i = 0; i < n; i++) {
    Kernel::sp() << IntRadix::Hex << "Segment group " << segment_groups[i].base << " " << segment_groups[i].segment_group << segment_groups[i].bus_start << " " << segment_groups[i].bus_end << "\n";
    auto base = segment_groups[i].base;
    if (base >= IDENTITY_MAP_START) {
      Kernel::sp() << "Warning segment group start addr out of range " << IntRadix::Hex << base << "\n";
      continue;
    }

    for (u64 j = 0; j < segment_groups[i].bus_end - segment_groups[i].bus_start; j++) {
      auto bus = segment_groups[i].bus_start + j;
      for (int device = 0; device < 32; device++) {
        auto cs0 = (ExtendedConfigSpace*)(KERNEL_START + (base | (j << 20) | (device << 15)));
        if (cs0->vendor == PCI_INVALID_VENDOR) {
          continue;
        }

        Kernel::sp()
            << IntRadix::Hex
            << " "
            << " bus 0x" << bus
            << " slot 0x" << device << "\n";

        for (int function = 0; function < 8; function++) {
          volatile auto cs = (ExtendedConfigSpace*)(KERNEL_START + (base | (j << 20) | (device << 15 | function << 12)));
          if (cs->vendor == PCI_INVALID_VENDOR) {
            continue;
          }
          Kernel::sp()
              << "    function 0x" << function
              << " vendor 0x" << cs->vendor
              << " device 0x" << cs->device
              << " header 0x" << cs->header_type
              << " class 0x" << cs->class_code
              << " subclass 0x" << cs->subclass
              << " IRQ line 0x" << cs->interrupt_line
              << " IRQ pin 0x" << cs->interrupt_pin;
          auto it = drivers_.find({cs->vendor, cs->device});
          if (it != drivers_.end()) {
            Kernel::sp() << " driver '" << it->second->name().c_str() << "'\n";
            PCIDeviceInfo info{cs};

            for (int bar_id = 0; bar_id < 6; bar_id++) {
              if (cs->bars[bar_id] != 0) {
                auto [start, size, is_io] = bar_parse(&cs->bars[bar_id]);
                info.bars[bar_id].start = start;
                info.bars[bar_id].size = size;
                info.bars[bar_id].is_io = is_io;
                Kernel::sp() << "      BAR[" << bar_id << "] = 0x" << IntRadix::Hex << start << ", size = 0x" << size << (is_io ? " IO space" : " Memory space") << "\n";
              }
            }

            // if successfully initialized, add to active drivers
            if (it->second->Enumerate(&info)) {
              active_drivers_.push_back(it->second.get());
            }
          } else {
            Kernel::sp() << " no driver\n";
          }
          // Intel ICH9, other AHCI compatible devices should also work
//            if (cs->vendor == 0x8086 && (cs->device == 0x2922 || cs->device == 0x2829)) {
//              abar = &cs->bars[5];
//            } else if (cs->vendor == 0x10ec && cs->device == 0x8139) {
//              void rtl8139_init(volatile ExtendedConfigSpace *, volatile void *io_base);
//              for (int bar_id = 0; bar_id < 6; bar_id++) {
//                if (cs->bars[bar_id] != 0) {
//                  auto [start, size, is_io] = bar_parse(&cs->bars[bar_id]);
//                  Kernel::sp() << "      BAR[" << bar_id << "] = 0x" << IntRadix::Hex << start << ", size = 0x" << size << (is_io ? " IO space" : " Memory space") << "\n";
//                }
//              }
//              // BAR1 contains memory address for registers
//              rtl8139_init(cs, (void*)(KERNEL_START + cs->bars[1]));
//            } else {
//              // disable IRQ for unknown devices
//              cs->command = (cs->command | (1<<10)) & ~(1<<2);
//            }
        }
      }
    }
  }
  Kernel::sp() << "PCI Enumeration done\n";

  Kernel::k->irq_->Register(0x40, 0x18, [this](u64 irq_num, u64 error_num, Context *context) {

    // COM1
    if (irq_num == 0x44) {
      u8 irq_status = inb(0x3f8 + 2);
      if ((irq_status & 1) == 0) {
        Kernel::sp() << "Serial IRQ\n";
        u8 reason = (irq_status & 0xe)>>1;
        if (reason == 6) {
          u8 data = inb(0x3f8 + 0);
          Kernel::sp() << "Serial IRQ timeout\n";
        } else if (reason == 2) {
          u8 data = inb(0x3f8 + 0);
          Kernel::sp() << "Serial Port input: " << (char)data << "\n";
        } else {
          Kernel::sp() << "Serial Port reason: " << (char)reason << "\n";
        }
      }
    }
    lapic_eoi();

    bool handled = false;
    for (auto d : active_drivers_) {
      if (d->HandleInterrupt(irq_num)) {
        handled = true;
        break;
      }
    }
  });
}
