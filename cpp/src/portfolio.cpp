#include "qflow/portfolio.hpp"

#include <cmath>

#include "qflow/instrument.hpp"

namespace qflow {

MoneyCents notional_cents(PriceTicks price, Quantity qty,
                          double tick_size) noexcept {
  const double dollars =
      to_price(price, tick_size) * static_cast<double>(qty);
  return static_cast<MoneyCents>(std::llround(dollars * 100.0));
}

Portfolio::Portfolio(MoneyCents initial_cash_cents)
    : cash_cents_(initial_cash_cents) {}

OrderId Portfolio::place(Order order, std::string* error_message) {
  if (order.instrument_id == 0) {
    if (error_message != nullptr) {
      *error_message = "instrument_id required";
    }
    return 0;
  }
  if (order.quantity <= 0) {
    if (error_message != nullptr) {
      *error_message = "quantity must be positive";
    }
    return 0;
  }
  if (order.type == OrderType::Limit && order.limit_price <= 0) {
    if (error_message != nullptr) {
      *error_message = "limit_price required for limit orders";
    }
    return 0;
  }

  order.id = next_order_id_++;
  order.filled = 0;
  order.status = OrderStatus::Accepted;
  const OrderId id = order.id;
  orders_.emplace(id, order);
  return id;
}

bool Portfolio::apply_fill(const Fill& fill, double tick_size,
                           std::string* error_message) {
  if (!is_valid(fill)) {
    if (error_message != nullptr) {
      *error_message = "invalid fill";
    }
    return false;
  }
  if (tick_size <= 0.0) {
    if (error_message != nullptr) {
      *error_message = "tick_size must be positive";
    }
    return false;
  }

  Order* order = find_order(fill.order_id);
  if (order == nullptr) {
    if (error_message != nullptr) {
      *error_message = "unknown order_id";
    }
    return false;
  }
  if (!is_open(*order)) {
    if (error_message != nullptr) {
      *error_message = "order is not open";
    }
    return false;
  }
  if (fill.instrument_id != order->instrument_id) {
    if (error_message != nullptr) {
      *error_message = "instrument mismatch";
    }
    return false;
  }
  if (fill.side != order->side) {
    if (error_message != nullptr) {
      *error_message = "side mismatch";
    }
    return false;
  }
  if (fill.quantity > leaves(*order)) {
    if (error_message != nullptr) {
      *error_message = "fill quantity exceeds leaves";
    }
    return false;
  }

  const MoneyCents money = notional_cents(fill.price, fill.quantity, tick_size);
  if (fill.side == Side::Buy) {
    if (money > cash_cents_) {
      if (error_message != nullptr) {
        *error_message = "insufficient cash";
      }
      return false;
    }
    cash_cents_ -= money;
  } else {
    cash_cents_ += money;
  }

  order->filled += fill.quantity;
  order->status =
      leaves(*order) == 0 ? OrderStatus::Filled : OrderStatus::PartiallyFilled;

  Position& pos = positions_[fill.instrument_id];
  apply_fill_to_position(pos, fill);
  return true;
}

const Order* Portfolio::find_order(OrderId id) const {
  const auto it = orders_.find(id);
  return it == orders_.end() ? nullptr : &it->second;
}

Order* Portfolio::find_order(OrderId id) {
  const auto it = orders_.find(id);
  return it == orders_.end() ? nullptr : &it->second;
}

Position Portfolio::position(InstrumentId id) const {
  const auto it = positions_.find(id);
  if (it == positions_.end()) {
    return Position{id, 0, 0};
  }
  return it->second;
}

std::vector<OrderId> Portfolio::open_orders() const {
  std::vector<OrderId> ids;
  for (const auto& entry : orders_) {
    if (is_open(entry.second)) {
      ids.push_back(entry.first);
    }
  }
  return ids;
}

}  // namespace qflow
