#pragma once

#include "qflow/order.hpp"
#include "qflow/types.hpp"

namespace qflow {

/** One execution against an order (full or partial). */
struct Fill {
  OrderId order_id{0};
  InstrumentId instrument_id{0};
  TimestampNs ts_ns{0};
  Side side{Side::Buy};
  PriceTicks price{0};
  Quantity quantity{0};
};

inline bool is_valid(const Fill& fill) noexcept {
  return fill.order_id != 0 && fill.instrument_id != 0 && fill.ts_ns > 0 &&
         fill.price > 0 && fill.quantity > 0;
}

}  // namespace qflow
