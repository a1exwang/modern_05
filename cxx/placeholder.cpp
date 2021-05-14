#include <lib/string.h>
#include <kernel.h>

namespace std {

[[maybe_unused]]
void __throw_bad_function_call() { // NOLINT(bugprone-reserved-identifier)
  Kernel::k->stack_dump(get_rbp());
  Kernel::k->panic("__throw_bad_function_call");
}
}


extern "C" {
  void __cxa_pure_virtual() {
    Kernel::k->stack_dump(get_rbp());
    Kernel::k->panic("__cxa_pure_virtual");
  }
//  void __stack_chk_fail() {
//    Kernel::k->stack_dump(get_rbp());
//    Kernel::k->panic("__stack_chk_fail");
//  }
}

