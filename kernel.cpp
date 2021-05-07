
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
#include <mm/mm.h>
#include "debug.h"
#include <init/efi_info.h>
#include <process.h>
#include <device/pci.h>

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
void process_init();

void apic_init();
void Kernel::start() {
  cli();

  SerialPort port;
  port << "Hello from kernel! \n";

  if (efi_info.kernel_physical_start % PAGE_SIZE != 0) {
    panic("Kernel start address not page aligned");
  }

  debug_init();
  mm_init();

  apic_init();
  pci_init();
  irq_init();
  process_init();

  return_from_syscall(current_context());

  panic("ERROR: kernel::start() should not return, but it returns");
}

__attribute__((noreturn))
void Kernel::panic(const char *s) {
  serial_port_ << "Kernel panic: '" << s << "'\n";
  dump_all();
  halt();
  __builtin_unreachable();
}
