#pragma once

#include <string>

#include "qflow/types.hpp"

namespace qflow {

/** Tradable instrument metadata used to interpret ticks and bars. */
struct Instrument {
  InstrumentId id{0};
  std::string symbol;
  /** Minimum price increment expressed in the same decimal scale as display prices. */
  double tick_size{0.01};
};

inline bool is_valid(const Instrument& instrument) noexcept {
  return instrument.id != 0 && !instrument.symbol.empty() && instrument.tick_size > 0.0;
}

/** Convert a human decimal price to integer tick units. */
inline PriceTicks to_ticks(double price, double tick_size) noexcept {
  return static_cast<PriceTicks>(price / tick_size + (price >= 0.0 ? 0.5 : -0.5));
}

/** Convert integer tick units back to a decimal price. */
inline double to_price(PriceTicks ticks, double tick_size) noexcept {
  return static_cast<double>(ticks) * tick_size;
}

}  // namespace qflow
