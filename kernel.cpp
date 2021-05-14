#include <init/efi_info.h>
#include <tuple>
#include <type_traits>
#include <lib/string.h>
#include <lib/port_io.h>
#include <lib/serial_port.h>
#include <cpu_utils.h>
#include <cpu_defs.h>
#include <kernel.h>
#include <irq.hpp>
#include <mm/mm.h>
#include "debug.h"
#include <init/efi_info.h>
#include <process.h>
#include <device/pci.h>
#include <syscall.h>
#include <mm/page_alloc.h>
#include <common/unwind.hpp>
#include <common/kmemory.hpp>
#include <fs/mem_fs_node.hpp>

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

extern u8* kernel_init_stack;
extern "C" u8* kernel_init_stack_bottom;

void Kernel::stacks_init() {
  stacks_.push_back(std::make_tuple((u64)kernel_init_stack, (u64)kernel_init_stack_bottom));
  stacks_.push_back(std::make_tuple((u64)interrupt_stack, (u64)interrupt_stack_bottom));
}

MemFsDirNode *Kernel::create_root_dir() {
  auto root = knew<MemFsDirNode>(nullptr);
  return root;
}

void Kernel::start() {
  cli();

  SerialPort port;
  port << "Hello from kernel! \n";

  if (efi_info.kernel_physical_start % PAGE_SIZE != 0) {
    panic("Kernel start address not page aligned");
  }

  // Init basic functionalities
  // You should not switch any order of these inits
  debug_init();
  mm_init();
  stacks_init();
  irq_ = irq_init();
  fs_root_ = create_root_dir();
  process_init();
  syscall_ = knew<Syscall>();

  // Init drivers
  apic_init();
  pci_init();

  // exec process 1
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

void Kernel::stack_dump(unsigned long rbp) {
  auto kstack = std::make_tuple(
      (u64)processes[current_pid]->kernel_stack,
      (u64)processes[current_pid]->kernel_stack_bottom
  );
  Kernel::sp() << "Stack dump:\n";
  Unwind unwind(rbp, Kernel::k->stacks_, kstack);
  unwind.Iterate([](u64 rbp, u64 ret_addr) {
    Kernel::sp() << "  0x" << ret_addr << " rbp 0x" << rbp << "\n";
    return true;
  });
}
