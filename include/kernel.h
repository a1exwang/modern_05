#pragma once

#include <lib/defs.h>
#include <lib/serial_port.h>
#include <cpu_utils.h>
#include <init/efi_info.h>

class Kernel {
 public:
  static Kernel *k;
  static SerialPort &sp() {
    return k->serial_port_;
  }

  void start();

  __attribute__((noreturn))
  void panic(const char *s);

 public:
  SerialPort serial_port_;
  EFIServicesInfo efi_info;
};
