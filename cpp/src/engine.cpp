#include "qflow/engine.hpp"

namespace qflow {

void EventEngine::post(Event event) {
  if (event.seq == 0) {
    event.seq = next_seq_++;
  } else if (event.seq >= next_seq_) {
    next_seq_ = event.seq + 1;
  }
  queue_.push(std::move(event));
}

std::size_t EventEngine::run() {
  std::size_t delivered = 0;
  while (!queue_.empty()) {
    Event event = queue_.top();
    queue_.pop();

    if (!clock_.advance_to(event.ts_ns)) {
      // Out-of-order relative to prior delivery; still dispatch but keep clock.
      // In a correct feed this should not happen after sorting by (ts, seq).
    }

    if (handler_ != nullptr) {
      handler_->on_event(event, clock_);
    }
    ++delivered;
  }
  return delivered;
}

}  // namespace qflow
