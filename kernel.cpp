
#include <init/efi_info.h>
#include <tuple>
#include <type_traits>
#include <string.h>
#include <lib/port_io.h>
#include <lib/serial_port.h>
#include <cpu_utils.h>
#include <cpu_defs.h>
#include <kernel.h>
#include <irq.h>
#include <mm.h>
#include "debug.h"

Kernel *Kernel::k;


extern "C"
void kernel_start() {
  EFIServicesInfo *ServicesInfo = (EFIServicesInfo*)EFIServiceInfoAddress;

  Kernel kernel;
  Kernel::k = &kernel;
  kernel.start();
}

void Kernel::start() {
  SerialPort port;
  port << "Hello from kernel! \n";

  debug_init();
  mm_init();
  irq_init();

  panic("kernel exits normally");
}
void Kernel::panic(const char *s) {
  serial_port_ << "Kernel panic: '" << s << "'\n";
  halt();
}
