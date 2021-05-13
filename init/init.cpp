#include <common/defs.h>
#include <init/efi_info.h>

extern "C" void kernel_start(EFIServicesInfo*);

u8 kernel_init_stack[4096 * 8];
// leave some safe zone
u8* kernel_init_stack_bottom = &kernel_init_stack[sizeof(kernel_init_stack)/sizeof(kernel_init_stack[0]) - 128];
extern "C" void _setup_init_stack();

extern "C" {

// kernel entrypoint
__attribute__((noreturn))
void init(EFIServicesInfo *efi_info) {
  _setup_init_stack();

  kernel_start(efi_info);
  __builtin_unreachable();
}

}
