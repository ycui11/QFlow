#pragma once

#include "qflow/types.hpp"

namespace qflow {

/**
 * Simulated clock for backtests and deterministic replay.
 * Only the engine should advance time; strategies read now().
 */
class SimClock {
 public:
  TimestampNs now() const noexcept { return now_; }

  void reset(TimestampNs ts_ns = 0) noexcept { now_ = ts_ns; }

  /** Advance to ts_ns if it is not in the past. Returns false if ts_ns < now_. */
  bool advance_to(TimestampNs ts_ns) noexcept {
    if (ts_ns < now_) {
      return false;
    }
    now_ = ts_ns;
    return true;
  }

 private:
  TimestampNs now_{0};
};

}  // namespace qflow
