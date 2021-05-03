#include <lib/defs.h>

extern "C" void kernel_start();

u8 kernel_init_stack[4096 * 8];
extern "C" u8* kernel_init_stack_bottom;
u8* kernel_init_stack_bottom = &kernel_init_stack[sizeof(kernel_init_stack)/sizeof(kernel_init_stack[0]) - 128];
extern "C" void _setup_init_stack();

extern "C" {

__attribute__((noreturn))
void init()
{
  _setup_init_stack();
  kernel_start();
  __builtin_unreachable();
}

}
