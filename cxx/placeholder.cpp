#include <lib/string.h>
#include <kernel.h>

namespace std {

[[maybe_unused]]
void __throw_bad_function_call() { // NOLINT(bugprone-reserved-identifier)
  Kernel::k->stack_dump(get_rbp());
  Kernel::k->panic("__throw_bad_function_call");
  __builtin_unreachable();
}
}

