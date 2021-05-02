#pragma once

#include <lib/defs.h>
#include <lib/serial_port.h>
#include <cpu_utils.h>

class Kernel {
 public:
  static Kernel *k;

  static SerialPort &sp() {
    return k->serial_port_;
  }

  void start();

  void panic(const char *s);

 public:
  SerialPort serial_port_;
};
