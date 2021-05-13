#pragma once

#include <common/defs.h>
#include <lib/serial_port.h>
#include <cpu_utils.h>
#include <init/efi_info.h>
#include <common/small_vec.hpp>
constexpr size_t MaxValidStacks = 8;

class InterruptProcessor;
class Syscall;

class Kernel {
 public:
  static Kernel *k;
  static SerialPort &sp() {
    return k->serial_port_;
  }

  void start();
  void stacks_init();

  __attribute__((noreturn))
  void panic(const char *s);

 public:
  // public services
  InterruptProcessor *irq_;
  Syscall *syscall_;

 public:
  SmallVec<std::tuple<u64, u64>, MaxValidStacks> stacks_;

  SerialPort serial_port_;
  EFIServicesInfo efi_info;
};
