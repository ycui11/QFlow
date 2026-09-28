#pragma once

#include "qflow/fill.hpp"
#include "qflow/types.hpp"

namespace qflow {

/** Net position for one instrument. quantity > 0 long, < 0 short. */
struct Position {
  InstrumentId instrument_id{0};
  Quantity quantity{0};
  PriceTicks avg_price{0};
};

inline bool is_flat(const Position& position) noexcept {
  return position.quantity == 0;
}

/** Apply a fill to a position (updates qty and average price). */
inline void apply_fill_to_position(Position& position, const Fill& fill) noexcept {
  const Quantity delta =
      fill.side == Side::Buy ? fill.quantity : -fill.quantity;
  const Quantity old_qty = position.quantity;
  const Quantity new_qty = old_qty + delta;

  const bool old_long = old_qty > 0;
  const bool add_long = delta > 0;
  if (old_qty == 0 || old_long == add_long) {
    const Quantity abs_old = old_qty >= 0 ? old_qty : -old_qty;
    const Quantity abs_add = fill.quantity;
    const Quantity denom = abs_old + abs_add;
    if (denom > 0) {
      position.avg_price =
          (position.avg_price * abs_old + fill.price * abs_add) / denom;
    }
  } else if (new_qty == 0) {
    position.avg_price = 0;
  }

  position.instrument_id = fill.instrument_id;
  position.quantity = new_qty;
}

}  // namespace qflow
