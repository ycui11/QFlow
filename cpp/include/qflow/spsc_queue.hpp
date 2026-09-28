#pragma once

#include <atomic>
#include <cstddef>
#include <utility>

namespace qflow {

/**
 * Single-producer single-consumer ring buffer (power-of-two capacity).
 * One thread may push; another may pop. Not safe for multiple producers.
 *
 * This is an educational SPSC queue — verify with your platform before
 * relying on it for production HFT.
 */
template <typename T, std::size_t Capacity>
class SpscQueue {
  static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of two");
  static_assert(Capacity >= 2, "Capacity must be >= 2");
 public:
  SpscQueue() = default;

  SpscQueue(const SpscQueue&) = delete;
  SpscQueue& operator=(const SpscQueue&) = delete;

  bool try_push(const T& item) {
    const std::size_t head = head_.load(std::memory_order_relaxed);
    const std::size_t next = (head + 1) & kMask;
    if (next == tail_.load(std::memory_order_acquire)) {
      return false;  // full
    }
    slots_[head] = item;
    head_.store(next, std::memory_order_release);
    return true;
  }

  bool try_push(T&& item) {
    const std::size_t head = head_.load(std::memory_order_relaxed);
    const std::size_t next = (head + 1) & kMask;
    if (next == tail_.load(std::memory_order_acquire)) {
      return false;
    }
    slots_[head] = std::move(item);
    head_.store(next, std::memory_order_release);
    return true;
  }

  bool try_pop(T& out) {
    const std::size_t tail = tail_.load(std::memory_order_relaxed);
    if (tail == head_.load(std::memory_order_acquire)) {
      return false;  // empty
    }
    out = std::move(slots_[tail]);
    tail_.store((tail + 1) & kMask, std::memory_order_release);
    return true;
  }

  bool empty() const noexcept {
    return tail_.load(std::memory_order_acquire) ==
           head_.load(std::memory_order_acquire);
  }

  static constexpr std::size_t capacity() noexcept { return Capacity; }

 private:
  static constexpr std::size_t kMask = Capacity - 1;

  alignas(64) std::atomic<std::size_t> head_{0};
  alignas(64) std::atomic<std::size_t> tail_{0};
  T slots_[Capacity]{};
};

}  // namespace qflow
