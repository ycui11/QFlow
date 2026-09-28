#pragma once

#include <cstdint>

#include "qflow/types.hpp"

namespace qflow {

using OrderId = std::uint64_t;

/** Integer currency in cents (1 dollar = 100). */
using MoneyCents = std::int64_t;

enum class OrderType : std::uint8_t {
  Market = 0,
  Limit = 1,
};

enum class OrderStatus : std::uint8_t {
  New = 0,
  Accepted = 1,
  PartiallyFilled = 2,
  Filled = 3,
  Rejected = 4,
  Cancelled = 5,
};

struct Order {
  OrderId id{0};
  InstrumentId instrument_id{0};
  Side side{Side::Buy};
  OrderType type{OrderType::Limit};
  OrderStatus status{OrderStatus::New};
  PriceTicks limit_price{0};
  Quantity quantity{0};
  Quantity filled{0};
  TimestampNs created_ts_ns{0};
};

inline Quantity leaves(const Order& order) noexcept {
  return order.quantity - order.filled;
}

inline bool is_open(const Order& order) noexcept {
  return order.status == OrderStatus::Accepted ||
         order.status == OrderStatus::PartiallyFilled;
}

inline bool is_done(const Order& order) noexcept {
  return order.status == OrderStatus::Filled ||
         order.status == OrderStatus::Rejected ||
         order.status == OrderStatus::Cancelled;
}

}  // namespace qflow
