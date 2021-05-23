#pragma once
#include <atomic>
#include <common/kvector.hpp>

// Single producer single consumer wait free queue
// one thread owns deq and another thread owns enq, no-reentry, only 2 threads
// queue is empty when start == end
// queue is full when end + 1 == start
template <typename T>
class SSQueue {
 public:
  // default initialize all elements
  explicit SSQueue(size_t size) :rx_queue_(size) {
    for (auto &item : rx_queue_) {
      item = knew<T>();
    }
  }
  explicit SSQueue(kvector<T*> vec) :rx_queue_(std::move(vec)) { }

  bool deq(T* &packet) {
    if (rx_start_ == rx_end_) {
      // empty queue
      return false;
    }

    std::swap<T*>(rx_queue_[rx_start_], packet);

    rx_start_ = (rx_start_ + 1) % rx_queue_.size();
    return true;
  }

  bool enq(T* &packet) {
    auto next = (rx_end_ + 1) % rx_queue_.size();
    if (next == rx_start_) {
      // queue full, drop new packet
      return false;
    }

    std::swap(packet, rx_queue_[rx_end_]);

    rx_end_ = next;
    return true;
  }

  kvector<T*> rx_queue_;
  std::atomic<size_t> rx_start_ = 0, rx_end_ = 0;
};