#include <lib/string.h>

namespace std {

[[maybe_unused]]
void __throw_bad_function_call() { // NOLINT(bugprone-reserved-identifier)
  assert(0, "Bad function call");
}

}