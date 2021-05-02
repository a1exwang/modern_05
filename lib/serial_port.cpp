#include "lib/serial_port.h"

SerialPort &operator<<(SerialPort &serial_port, const char *s) {
  for (int i = 0; s[i]; i++) {
    serial_port.put(s[i]);
  }
  return serial_port;
}

SerialPort &operator<<(SerialPort &serial_port, SerialPort::IntRadix radix) {
  serial_port.SetRadix(radix);
  return serial_port;
}
SerialPort &operator<<(SerialPort &serial_port, char c) {
  serial_port.put(c);
  return serial_port;
}
// T is int type
