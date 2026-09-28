#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "qflow/types.hpp"

namespace qflow {

/** Simple SMA over the last N prices (double domain for research/signals). */
class Sma {
 public:
  explicit Sma(std::size_t period);

  void reset();
  void update(double price);
  bool ready() const noexcept { return values_.size() >= period_; }
  double value() const noexcept { return value_; }
  std::size_t period() const noexcept { return period_; }

 private:
  std::size_t period_{0};
  std::vector<double> values_{};
  double sum_{0.0};
  double value_{0.0};
};

/** Exponential moving average: alpha = 2 / (period + 1). */
class Ema {
 public:
  explicit Ema(std::size_t period);

  void reset();
  void update(double price);
  bool ready() const noexcept { return primed_; }
  double value() const noexcept { return value_; }

 private:
  double alpha_{0.0};
  double value_{0.0};
  bool primed_{false};
};

/**
 * Emits true once when fast EMA crosses above slow EMA (bullish cross).
 * Call update() with each new price; check crossed_up() after update.
 */
class EmaCrossSignal {
 public:
  EmaCrossSignal(std::size_t fast_period, std::size_t slow_period);

  void reset();
  void update(double price);
  bool ready() const noexcept { return fast_.ready() && slow_.ready(); }
  bool crossed_up() const noexcept { return crossed_up_; }
  bool crossed_down() const noexcept { return crossed_down_; }
  double fast() const noexcept { return fast_.value(); }
  double slow() const noexcept { return slow_.value(); }

 private:
  Ema fast_;
  Ema slow_;
  bool have_prev_{false};
  double prev_fast_{0.0};
  double prev_slow_{0.0};
  bool crossed_up_{false};
  bool crossed_down_{false};
};

inline double ticks_to_double(PriceTicks ticks, double tick_size) noexcept {
  return static_cast<double>(ticks) * tick_size;
}

}  // namespace qflow
