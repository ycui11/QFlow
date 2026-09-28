#pragma once

#include "qflow/types.hpp"

namespace qflow {

/**
 * A single market print / last-trade style update.
 *
 * Field order is chosen to reduce padding: three 8-byte fields first,
 * then the 4-byte id and 1-byte side. On common ABIs this is 32 bytes
 * (see layout.hpp), so denser arrays touch fewer cache lines.
 */
struct Tick {
  TimestampNs ts_ns{0};
  PriceTicks price{0};
  Quantity size{0};
  InstrumentId instrument_id{0};
  Side side{Side::Buy};
};

inline bool is_valid(const Tick& tick) noexcept {
  return tick.ts_ns > 0 && tick.instrument_id != 0 && tick.price > 0 && tick.size > 0;
}

}  // namespace qflow
