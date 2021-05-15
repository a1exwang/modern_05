#pragma once

#include <atomic>

class kspinlock {
 public:
  kspinlock() { lock_.clear(); }
  kspinlock(const kspinlock&) = delete;
  ~kspinlock() = default;

  void lock() {
    while (lock_.test_and_set(std::memory_order_acquire));
  }
  bool try_lock() {
    return !lock_.test_and_set(std::memory_order_acquire);
  }
  void unlock() {
    lock_.clear(std::memory_order_release);
  }
 private:
  std::atomic_flag lock_;
};