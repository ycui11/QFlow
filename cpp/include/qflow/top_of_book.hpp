#pragma once

#include "qflow/types.hpp"

namespace qflow {

/**
 * Top of book (BBO): best bid and best ask only.
 * A full multi-level order book comes in a later lesson; this is enough
 * to reason about spread and mid price on the hot path.
 */
struct TopOfBook {
  TimestampNs ts_ns{0};
  InstrumentId instrument_id{0};
  PriceTicks bid_price{0};
  Quantity bid_size{0};
  PriceTicks ask_price{0};
  Quantity ask_size{0};
};

inline bool is_valid(const TopOfBook& book) noexcept {
  return book.ts_ns > 0 && book.instrument_id != 0 && book.bid_price > 0 &&
         book.ask_price > 0 && book.ask_price >= book.bid_price &&
         book.bid_size > 0 && book.ask_size > 0;
}

inline PriceTicks mid_ticks(const TopOfBook& book) noexcept {
  return (book.bid_price + book.ask_price) / 2;
}

inline PriceTicks spread_ticks(const TopOfBook& book) noexcept {
  return book.ask_price - book.bid_price;
}

}  // namespace qflow
