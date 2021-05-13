#include <common/unwind.hpp>

bool Unwind::ValidRbp(unsigned long rbp) const {
  auto [start, end] = kthread_stack_range;
  if (start <= rbp && rbp + 16 <= end) {
    return true;
  }
  for (auto [start, end] : valid_stacks) {
    if (start <= rbp && rbp + 16 <= end) {
      return true;
    }
  }
  return false;
}

void Unwind::Iterate(std::function<bool(u64, u64)> callback) {
  u64 prev_rbp, ret_addr;

  while (true) {
    if (!ValidRbp(rbp)) {
      break;
    }

    prev_rbp = *(u64*)rbp;
    ret_addr = *(u64*)(rbp + 8);

    if (!callback(prev_rbp, ret_addr)) {
      break;
    }

    rbp = prev_rbp;
  }
}
