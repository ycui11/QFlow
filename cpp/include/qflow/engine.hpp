#pragma once

#include <cstddef>
#include <queue>
#include <vector>

#include "qflow/clock.hpp"
#include "qflow/event.hpp"

namespace qflow {

/** Observer invoked for every dispatched event (strategy / logger / bookkeeping). */
class EventHandler {
 public:
  virtual ~EventHandler() = default;
  virtual void on_event(const Event& event, const SimClock& clock) = 0;
};

/**
 * Single-threaded event loop: post events, then run() drains them in time order.
 * Deterministic: (ts_ns, seq) ordering makes replays stable.
 */
class EventEngine {
 public:
  void set_handler(EventHandler* handler) noexcept { handler_ = handler; }

  EventHandler* handler() const noexcept { return handler_; }

  const SimClock& clock() const noexcept { return clock_; }

  SimClock& clock() noexcept { return clock_; }

  /** Queue an event. If seq == 0, a monotonic sequence number is assigned. */
  void post(Event event);

  std::size_t pending() const noexcept { return queue_.size(); }

  bool empty() const noexcept { return queue_.empty(); }

  /**
   * Dispatch all pending events in order.
   * Returns the number of events delivered to the handler.
   */
  std::size_t run();

  void clear() {
    queue_ = {};
    next_seq_ = 1;
    clock_.reset(0);
  }

 private:
  struct LaterFirst {
    bool operator()(const Event& a, const Event& b) const noexcept {
      if (a.ts_ns != b.ts_ns) {
        return a.ts_ns > b.ts_ns;
      }
      return a.seq > b.seq;
    }
  };

  using Queue = std::priority_queue<Event, std::vector<Event>, LaterFirst>;

  Queue queue_{};
  SimClock clock_{};
  EventHandler* handler_{nullptr};
  std::uint64_t next_seq_{1};
};

}  // namespace qflow
