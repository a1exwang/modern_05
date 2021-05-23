#pragma once

#include <tuple>

#include <type_traits>
#include <common/defs.h>
#include <lib/port_io.h>

enum class IntRadix {
  Binary,
  Oct,
  Dec,
  Hex
};


/**
 * COM serial port
 * https://wiki.osdev.org/Serial_Ports
 */
class SerialPort {
 public:
  using IntRadix = ::IntRadix;
  explicit SerialPort(u16 port = 0x3f8) :port(port) {
    outb(port + 1, 0x01);    //
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

SerialPort &operator<<(SerialPort &serial_port, const char *s);


SerialPort &operator<<(SerialPort &serial_port, const char *s);
SerialPort &operator<<(SerialPort &serial_port, SerialPort::IntRadix radix);
SerialPort &operator<<(SerialPort &serial_port, char c);

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
