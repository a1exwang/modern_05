#include <tuple>
#include <type_traits>

#include <cpu_defs.h>
#include <cpu_utils.h>
#include <irq.hpp>
#include <kernel.h>
#include <syscall.h>

#include <common/kmemory.hpp>
#include <common/unwind.hpp>
#include <device/serial8250.hpp>
#include <device/pci.h>
#include <device/pci_devices.hpp>
#include <fs/mem_fs_node.hpp>
#include <init/efi_info.h>
#include <lib/port_io.h>
#include <lib/serial_port.h>
#include <lib/string.h>
#include <mm/mm.h>
#include <mm/page_alloc.h>
#include <process.h>
#include "debug.h"

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
void efi_table_init();

void lapic_init();

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

typedef void (*InitFunc)();
extern "C" InitFunc _INIT_ARRAY_START_[]; // NOLINT(bugprone-reserved-identifier)
extern "C" InitFunc _INIT_ARRAY_END_[]; // NOLINT(bugprone-reserved-identifier)

class GlobalConstructorTest {
 public:
  GlobalConstructorTest() {
    Kernel::sp() << "    Testing global C++ initialization\n";
  }
};

__attribute__((used))
GlobalConstructorTest _test_global_init; // NOLINT(bugprone-reserved-identifier)

__attribute__((constructor, used))
static void test_c_init_func() {
  Kernel::sp() << "    Testing GCC's constructor attribute initialization\n";
}

void Kernel::start() {
  cli();

  SerialPort port;
  port << "Hello from kernel! \n";

  if (efi_info.kernel_physical_start % PAGE_SIZE != 0) {
    panic("Kernel start address not page aligned");
  }

  // We do similar things to what GCC does
  // https://github.com/gcc-mirror/gcc/blob/16e2427f50c208dfe07d07f18009969502c25dc8/libgcc/config/ia64/crtbegin.S
  // We won't be able to link GCC's crti.o crtbegin.o because of relocation issues
  // All the global/static variable constructors and __attribute__((constructor)) functions will be called here
  size_t n = _INIT_ARRAY_END_ - _INIT_ARRAY_START_;
  port << "Calling global initilizers\n";
  for (size_t i = 0; i < n; i++) {
    port << "  before calling ctor " << i << " at 0x" << IntRadix::Hex << (u64)_INIT_ARRAY_START_[i] << "\n";
    _INIT_ARRAY_START_[i]();
  }

  // Init basic functionalities
  // You should not switch any order of these inits
  debug_init();
  mm_init();
  stacks_init();
  irq_ = irq_init();
  fs_root_ = create_root_dir();
  process_init();
  Syscall::SetupSyscall(this);

  // Init drivers
  lapic_init();

  pci_bus_driver_ = knew<PCIBusDriver>();
  register_pci_devices(*pci_bus_driver_);
  // this will also init pci bus
  efi_table_init();

  com1_ = knew<Serial8250>(0x3f8);

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
