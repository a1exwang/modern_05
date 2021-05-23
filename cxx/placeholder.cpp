#include <lib/string.h>
#include <kernel.h>

namespace std {

[[maybe_unused]]
void __throw_bad_function_call() { // NOLINT(bugprone-reserved-identifier)
  Kernel::k->stack_dump(get_rbp());
  Kernel::k->panic("__throw_bad_function_call");
}
[[maybe_unused]]
void __throw_length_error(char const*) {
  Kernel::k->stack_dump(get_rbp());
  Kernel::k->panic("__throw_length_error");
}
[[maybe_unused]]
void __throw_logic_error(char const*) {
  Kernel::k->stack_dump(get_rbp());
  Kernel::k->panic("__throw_logic_error");
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
void abort() {
  Kernel::k->stack_dump(get_rbp());
  Kernel::k->panic("abort");
}
}

