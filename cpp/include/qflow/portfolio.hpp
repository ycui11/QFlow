#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "qflow/fill.hpp"
#include "qflow/order.hpp"
#include "qflow/position.hpp"

namespace qflow {

/**
 * Convert price_ticks * qty into integer cents using tick_size.
 * Example: 19025 ticks, tick_size 0.01, qty 100 -> $19025.00 -> 1_902_500 cents.
 */
MoneyCents notional_cents(PriceTicks price, Quantity qty, double tick_size) noexcept;

/**
 * Simple single-currency portfolio: cash + positions + working orders.
 * Matching / strategy live elsewhere; this only books orders and fills.
 */
class Portfolio {
 public:
  explicit Portfolio(MoneyCents initial_cash_cents);

  MoneyCents cash_cents() const noexcept { return cash_cents_; }

  OrderId place(Order order, std::string* error_message = nullptr);

  bool apply_fill(const Fill& fill, double tick_size,
                  std::string* error_message = nullptr);

  const Order* find_order(OrderId id) const;
  Order* find_order(OrderId id);

  Position position(InstrumentId id) const;

  const std::unordered_map<InstrumentId, Position>& positions() const noexcept {
    return positions_;
  }

  std::vector<OrderId> open_orders() const;

 private:
  MoneyCents cash_cents_{0};
  OrderId next_order_id_{1};
  std::unordered_map<OrderId, Order> orders_;
  std::unordered_map<InstrumentId, Position> positions_;
};

}  // namespace qflow
