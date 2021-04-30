
#include <init/efi_info.h>
#include <tuple>
#include <type_traits>

#define IA32_APIC_BASE_MSR 0x1B

void memset(void *data, u8 value, u64 size) {
  for (u64 i = 0; i < size; i++) {
    ((u8*)data)[i] = value;
  }
}

void outb(u16 address, u8 data) {
  asm (
  "mov %1, %%al\n"
  "mov %0, %%dx\n"
  "outb %%al, %%dx\n"
  :
  : "r"(address), "r"(data)
  : "%dx", "%al"
  );
}
u8 inb(u16 address) {
  u8 result = 0;
  asm (
  "mov %1, %%dx\n"
  "inb %%dx, %%al\n"
  "mov %%al, %0\n"
  : "=r"(result)
  : "r"(address)
  : "%dx", "%al"
  );
  return result;
}

u64 cpuGetMSR(u32 msr) {
  u64 result;
  asm volatile("rdmsr" : "=a"(*(u32*)&result), "=d"(*(u32*)((char*)&result+4)) : "c"(msr));
  return result;
}

void cpuSetMSR(u32 msr, u64 value) {
  asm volatile("wrmsr" : : "a"(value & 0xffffffff), "d"(value >> 32), "c"(msr));
}

extern "C" void _default_irq_handler();

/**
 * COM serial port
 * https://wiki.osdev.org/Serial_Ports
 */
class SerialPort {
 public:
  enum class IntRadix {
    Binary,
    Oct,
    Dec,
    Hex
  };
  explicit SerialPort(u16 port = 0x3f8) :port(port) {
    outb(port + 1, 0x00);    // Disable all interrupts
    outb(port + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(port + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    outb(port + 1, 0x00);    //                  (hi byte)
    outb(port + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(port + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    outb(port + 4, 0x0B);    // IRQs enabled, RTS/DSR set
  }

  void SetRadix(IntRadix radix) {
    int_radix_ = radix;
  }

  int is_transmit_empty() const {
    return inb(port + 5) & 0x20;
  }

  int serial_received() const {
    return inb(port + 5) & 1;
  }

  void put(u8 byte) {
    while (is_transmit_empty() == 0);
    outb(port, byte);
  }

  bool try_put(u8 byte) {
    if (!is_transmit_empty()) {
      put(byte);
      return true;
    } else {
      return false;
    }
  }

  u8 get() {
    while (!serial_received());
    return inb(port);
  }

  std::tuple<bool, u8> try_get() {
    if (serial_received()) {
      return {true, get()};
    } else {
      return {false, 0};
    }
  }

  template <typename T>
  friend SerialPort &operator<<(SerialPort &serial_port, T);

 private:
  u16 port;
  IntRadix int_radix_;
};

SerialPort &operator<<(SerialPort &serial_port, const char *s) {
  for (int i = 0; s[i]; i++) {
    serial_port.put(s[i]);
  }
  return serial_port;
}

SerialPort &operator<<(SerialPort &serial_port, char c) {
  serial_port.put(c);
  return serial_port;
}
// T is int type
template <typename T>
SerialPort &operator<<(SerialPort &serial_port, T n) {
  static_assert(std::is_integral_v<T>);
  // 20 digits is enough for 2**31-1
  char temp[20];

  const char *alphabet = "0123456789abcdef";

  T remain = n;
  int radix = 10;
  if (serial_port.int_radix_ == SerialPort::IntRadix::Hex) {
    radix = 16;
  } else if (serial_port.int_radix_ == SerialPort::IntRadix::Dec) {
    radix = 10;
  } else {
    // TODO: panic
  }
  if (n == 0) {
    serial_port.put('0');
  } else {
    int i = 0;
    while (remain) {
      auto newRemain = remain / radix;
      auto digit = alphabet[remain % radix];
      temp[i] = digit;
      i++;

      remain = newRemain;
    }
    while (i > 0) {
      i--;
      serial_port.put(temp[i]);
    }
  }
  return serial_port;
}

SerialPort &operator<<(SerialPort &serial_port, SerialPort::IntRadix radix) {
  serial_port.SetRadix(radix);
  return serial_port;
}

#define CR4_PSE (1u<<4u)
#define CR4_PAE (1u<<5u)

// 9 9 9 9
// 512G 1G 2M 4K
u64 pml4t[512] = {0};
u64 pdpt[512] = {0};
u64 pdt[512] = {0};
u64 pt[512] = {0};

// +0xffff8000 00000000 mapping
// 1M~33M -> kernel image
// 33M~40M -> image data structures/non swappable

// assert phy_start, phy_end aligned to 4k
void setup_kernel_page_table(u64 phy_start, u64 phy_end) {
  // kernel start + 1G
  pml4t[0] = pml4t[256] = reinterpret_cast<u64>(pdpt);

}

static bool is_page_present(u64 page_table_entry) {
  return page_table_entry & 0x1u;
}

#define PRINT_REG(type, name) \
  do {                      \
    type reg_##name = 0; \
    asm volatile("mov %%" #name ",%0" : "=r" (reg_##name)); \
    serial_port_ << #name " = 0x" << SerialPort::IntRadix::Hex << reg_##name << "\n"; \
  } while (0)

#pragma pack(push, 1)
struct SegmentDescriptor {
  u16 segment_limit_lo16;
  u16 base_addr_lo16;
  u8 base_addr_mid8;

  u8 accessed : 1;
  u8 wr : 1;
  u8 dc : 1;
  u8 exec : 1;
  u8 reserved : 1;
  u8 dpl : 2;
  u8 present : 1;

  u8 segment_limit_hi4 : 4;

  u8 avl : 1;
  u8 long_mode : 1;
  u8 default_operand_size : 1;
  u8 granularity : 1;

  u8 base_addr_hi8;
};

enum DescriptorType {
  LongLDT = 0x2,
  LongAvailableTSS = 0x9,
  LongBusyTSS = 0xb,
  LongCallGate = 0xc,
  LongInterruptGate = 0xe,
  LongTrapGate = 0xf,
};

struct InterruptDescriptor {
  u16 offset_lo16;
  u16 selector;

  u8 ist : 4;
  u8 zero : 4;
  u8 type : 4;

  u8 zero2 : 1;
  u8 dpl : 2;
  u8 p : 1;

  u16 offset_mid16;
  u32 offset_hi32;
  u32 zero3;
};
#pragma pack(pop)

InterruptDescriptor main_idt[256];

void set_idt_offset(InterruptDescriptor *desc, void* ptr) {
  desc->offset_lo16 = (u64)ptr & 0xffff;
  desc->offset_mid16 = ((u64)(ptr) >> 16) & 0xffff;
  desc->offset_hi32 = ((u64)(ptr) >> 32) & 0xffffffff;
}

extern "C" void _de_irq_handler();
extern "C" void _db_irq_handler();
extern "C" void _nmi_irq_handler();
extern "C" void _bp_irq_handler();

extern "C" void _timer_irq_handler();

void setup_idt(u16 selector, void *default_handler) {
  memset(main_idt, 0, sizeof(main_idt));

  for (int i = 0; i < sizeof(main_idt) / sizeof(main_idt[0]); i++) {
    set_idt_offset(&main_idt[i], default_handler);

    main_idt[i].selector = selector;
    main_idt[i].type = DescriptorType::LongInterruptGate;
    main_idt[i].dpl = 0;
    main_idt[i].p = 1;
  }
  set_idt_offset(&main_idt[0], (void*)&_de_irq_handler);
  set_idt_offset(&main_idt[1], (void*)&_db_irq_handler);
  set_idt_offset(&main_idt[2], (void*)&_nmi_irq_handler);
  set_idt_offset(&main_idt[3], (void*)&_bp_irq_handler);
  set_idt_offset(&main_idt[32], (void*)&_timer_irq_handler);
}

void print_segment_descriptor(SerialPort &serial_port, u16 selector, void *gdt) {
  auto &desc = *reinterpret_cast<const SegmentDescriptor*>((char*)gdt+ (selector & 0xfff8));

  serial_port << "Segment descriptor 0x" << SerialPort::IntRadix::Hex << selector << " ";
  u32 base_addr = (u32)desc.base_addr_lo16 | ((u32)desc.base_addr_mid8 << 16) | ((u32)desc.base_addr_hi8 << 24);

//  serial_port << "  base_addr 0x" << SerialPort::IntRadix::Hex << base_addr;
  serial_port << " dpl " << SerialPort::IntRadix::Hex << desc.dpl << " ";

//  serial_port << "  avl 0x" << SerialPort::IntRadix::Hex << desc.dpl;
  serial_port << "long_mode " << SerialPort::IntRadix::Hex << desc.long_mode << " ";
  serial_port << "default_operand_size " << SerialPort::IntRadix::Hex << desc.default_operand_size << "\n";
//  serial_port << "  granularity 0x" << SerialPort::IntRadix::Hex << desc.granularity;
}

void print_idt_descriptor(SerialPort &serial_port, u16 index, const InterruptDescriptor *desc) {
  serial_port << "Interrupt descriptor 0x" << SerialPort::IntRadix::Hex << index << " ";
  u64 offset = desc->offset_lo16 | ((u64)desc->offset_mid16 << 16) | ((u64)desc->offset_hi32 << 32);
  serial_port << "present " << desc->p << SerialPort::IntRadix::Hex << " offset 0x" << offset << " selector 0x" << desc->selector << " type 0x" << desc->type << " dpl " << desc->dpl << "\n";
}

class Kernel {
 public:
  static Kernel *k;
  void start() {
    SerialPort port;
    port << "Hello from kernel! \n";
    print_regs();
    panic("kernel exits normally");
  }

  void halt() {
    while (true) ;
    //asm volatile ("hlt");
  }

  void panic(const char *s) {
    serial_port_ << "Kernel panic: '" << s << "'\n";
    halt();
  }

  std::tuple<void*, u16> get_gdt() {
    u8 gdtr[10];
    __asm__ __volatile__("sgdt %0"
      :"=m"(gdtr)
      :
      :"memory");

    u16 limit = *(u16*)&gdtr[0];
    void* offset = (void*)*(u64*)&gdtr[2];
    return std::make_tuple(offset, limit);
  }

  std::tuple<void*, u16> get_idt() {
    u8 idtr[10];
    __asm__ __volatile__("sidt %0"
    :"=m"(idtr)
    :
    :"memory");

    u16 limit = *(u16*)&idtr[0];
    void* offset = (void*)*(u64*)&idtr[2];
    return std::make_tuple(offset, limit);
  }

  void load_idt(std::tuple<void*, u16> idtr_input) {
    u8 idtr[10];
    auto [offset, limit] = idtr_input;

    *(u16*)&idtr[0] = limit;
    *(u64*)&idtr[2] = (u64)offset;
    __asm__ __volatile__("lidt %0"
    :
    :"m"(idtr)
    :"memory");
  }

  void print_regs() {
    PRINT_REG(u16, cs);
    PRINT_REG(u16, fs);
    PRINT_REG(u16, gs);

    PRINT_REG(u64, rax);
    PRINT_REG(u64, rcx);
    PRINT_REG(u64, rdx);
    PRINT_REG(u64, rbx);
    PRINT_REG(u64, rsi);
    PRINT_REG(u64, rdi);
    PRINT_REG(u64, rbp);
    PRINT_REG(u64, rsp);
    u64 rflags = 0;
    __asm __volatile(
    "pushf\n"
    "pop %0\n"
    :"=m"(rflags)
    :
    :"memory"
    );

    serial_port_ << "rflags = 0x" << SerialPort::IntRadix::Hex << rflags << "\n"; \

    PRINT_REG(u64, cr0);
    PRINT_REG(u64, cr3);
    PRINT_REG(u64, cr4);

    auto [gdt_addr, _] = get_gdt();
    serial_port_ << "GDT address = 0x" << SerialPort::IntRadix::Hex << (u64)gdt_addr << '\n';

    u16 cs = 0;
    asm volatile("mov %%cs ,%0" : "=r" (cs));
    print_segment_descriptor(serial_port_, cs, gdt_addr);

    auto [idt_addr, idt_size] = get_idt();
    serial_port_ << "IDT address = 0x" << SerialPort::IntRadix::Hex << (u64)idt_addr << " limit 0x" << idt_size << '\n';
    for (u32 i = 0; i < 256; i++) {
      auto idt = reinterpret_cast<InterruptDescriptor *>(idt_addr);
      print_idt_descriptor(serial_port_, i, &idt[i]);
    }

    u64 cr3 = 0;
    asm volatile("mov %%cr3,%0" : "=r" (cr3));
    auto pml4t = reinterpret_cast<const u64*>(cr3 & 0xfffffffffffff000);
    // 9 | 9 | 9 | 9 | 12
    u64 total_pages = 0;
    for (int i = 0; i < 512; i++) {
      if (is_page_present(pml4t[i])) {
        auto pdpt = reinterpret_cast<const u64*>(pml4t[i] & 0xfffffffffffff000);
        for (int j = 0; j < 512; j++) {
          if (is_page_present(pdpt[j])) {
            auto pdt = reinterpret_cast<const u64*>(pdpt[i] & 0xfffffffffffff000);
            for (int k = 0; k < 512; k++) {
              if (is_page_present(pdt[k])) {
                auto pt = reinterpret_cast<const u64*>(pdt[k] & 0xfffffffffffff000);
                for (int l = 0; l < 512; l++) {
                  if (is_page_present(pt[l])) {
                    total_pages++;
                  }
                }
              }
            }
          }
        }
      }
    }
    serial_port_ << "total pages: 0x" << SerialPort::IntRadix::Hex << total_pages << '\n';

    setup_idt(cs, (void*)&_default_irq_handler);

    load_idt(std::make_tuple<void*, u16>(main_idt, 0xfff));
    __asm__ __volatile__("int $32");

    void* apic_base = (void*)cpuGetMSR(IA32_APIC_BASE_MSR);
    serial_port_ << "APIC base: 0x" << SerialPort::IntRadix::Hex << (u64)apic_base << '\n';
  }

 public:
  SerialPort serial_port_;
};
Kernel *Kernel::k;


extern "C"
void kernel_start() {
  EFIServicesInfo *ServicesInfo = (EFIServicesInfo*)EFIServiceInfoAddress;

  Kernel kernel;
  Kernel::k = &kernel;
  kernel.start();
}

extern "C" void timer_irq_handler() {
  Kernel::k->serial_port_ << "Timer IRQ " << "\n";
}

extern "C" void irq_handler(u64 irq_num, u64 error_code) {
  Kernel::k->serial_port_ << "IRQ = " << irq_num << " error = " << error_code << "\n";
  if (irq_num == 32) {
    timer_irq_handler();
    u64 esp;
    register u64 esp_val asm("esp");
    Kernel::k->serial_port_ << "esp = " << esp_val << "\n";
  }
}
