#pragma once

#include "qflow/types.hpp"

namespace qflow {

/** Aggregated OHLCV bar over a fixed interval. */
struct Bar {
  TimestampNs start_ts_ns{0};
  TimestampNs end_ts_ns{0};
  InstrumentId instrument_id{0};
  PriceTicks open{0};
  PriceTicks high{0};
  PriceTicks low{0};
  PriceTicks close{0};
  Quantity volume{0};
};

inline bool is_valid(const Bar& bar) noexcept {
  return bar.start_ts_ns > 0 && bar.end_ts_ns >= bar.start_ts_ns &&
         bar.instrument_id != 0 && bar.open > 0 && bar.high > 0 && bar.low > 0 &&
         bar.close > 0 && bar.high >= bar.low && bar.volume >= 0;
}

}  // namespace qflow
