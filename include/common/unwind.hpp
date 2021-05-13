#pragma once
#include <common/small_vec.hpp>
#include <functional>
#include <kernel.h>


class Unwind {
 public:
  Unwind(u64 rbp,
         const SmallVec<std::tuple<u64, u64>, MaxValidStacks> &valid_stacks,
         std::tuple<u64, u64> kthread_stack_range)
    :rbp(rbp), valid_stacks(valid_stacks),
    kthread_stack_range(kthread_stack_range) {
  }

  bool ValidRbp(u64 rbp) const;

  void Iterate(std::function<bool (u64 rbp, u64 ret_addr)> callback);

 private:
  u64 rbp;
  SmallVec<std::tuple<u64, u64>, MaxValidStacks> valid_stacks;
  std::tuple<u64, u64> kthread_stack_range;
};
