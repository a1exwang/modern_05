
#include <init/efi_info.h>
#include <tuple>
#include <type_traits>

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
      serial_port.put(temp[i]);
      i--;
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

class Kernel {
 public:
  static Kernel *k;
  void start() {
    SerialPort port;
    port << "Hello from kernel! \n";
    print_control_registers();
    panic("kernel exits normally");
  }

  void halt() {
    asm volatile ("hlt");
  }

  void panic(const char *s) {
    serial_port_ << "Kernel panic: '" << s << "'\n";
    halt();
  }
  void print_control_registers() {
    u64 cr0 = 0;
    asm volatile("mov %%cr0,%0" : "=r" (cr0));
    serial_port_ << "cr0 = 0x" << SerialPort::IntRadix::Hex << cr0 << "\n";

    u64 cr3 = 0;
    asm volatile("mov %%cr3,%0" : "=r" (cr3));
    serial_port_ << "cr3 = 0x" << SerialPort::IntRadix::Hex << cr3 << "\n";

    u64 cr4 = 0;
    asm volatile("mov %%cr4,%0" : "=r" (cr4));
    serial_port_ << "cr4 = 0x" << SerialPort::IntRadix::Hex << cr4 << "\n";

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
  }


 private:
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