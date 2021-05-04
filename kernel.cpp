
#include <init/efi_info.h>
#include <tuple>
#include <type_traits>
#include <lib/string.h>
#include <lib/port_io.h>
#include <lib/serial_port.h>
#include <cpu_utils.h>
#include <cpu_defs.h>
#include <kernel.h>
#include <irq.h>
#include <mm.h>
#include "debug.h"
#include <init/efi_info.h>

Kernel *Kernel::k;


extern "C" {

__attribute__((noreturn))
void kernel_start(EFIServicesInfo *efi_info) {
  Kernel kernel;
  memcpy(&kernel.efi_info, efi_info, sizeof(kernel.efi_info));
  Kernel::k = &kernel;
  kernel.start();
  __builtin_unreachable();
}

}
void thread_start();

void Kernel::start() {
  SerialPort port;
  port << "Hello from kernel! \n";

  debug_init();
  mm_init();
  irq_init();
  thread_start();

  panic("ERROR: kernel::start() should not return, but it returns");
}
void Kernel::panic(const char *s) {
  serial_port_ << "Kernel panic: '" << s << "'\n";
  dump_all();
  halt();
}
