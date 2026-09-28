#include "qflow/risk.hpp"

#include "qflow/portfolio.hpp"

namespace qflow {

RiskGate::RiskGate(RiskLimits limits) : limits_(limits) {}

RiskDecision RiskGate::evaluate(const Portfolio& portfolio, const Order& order,
                                double tick_size) const {
  if (!trading_enabled_) {
    return {false, "trading disabled (kill switch)"};
  }
  if (order.instrument_id == 0) {
    return {false, "instrument_id required"};
  }
  if (order.quantity <= 0) {
    return {false, "quantity must be positive"};
  }
  if (limits_.max_order_qty > 0 && order.quantity > limits_.max_order_qty) {
    return {false, "max_order_qty exceeded"};
  }
  if (limits_.max_orders_per_session > 0 &&
      accepted_count_ >= limits_.max_orders_per_session) {
    return {false, "max_orders_per_session exceeded"};
  }

  if (limits_.max_order_notional_cents > 0) {
    PriceTicks px = order.limit_price;
    if (order.type == OrderType::Market) {
      // Market orders: require a conservative placeholder — reject if no limit
      // and notional check is enabled without a price hint.
      if (px <= 0) {
        return {false, "market order needs limit_price hint for notional check"};
      }
    }
    if (px <= 0) {
      return {false, "price required for notional check"};
    }
    const MoneyCents notion = notional_cents(px, order.quantity, tick_size);
    if (notion > limits_.max_order_notional_cents) {
      return {false, "max_order_notional_cents exceeded"};
    }
  }

  if (limits_.max_abs_position > 0) {
    const Quantity cur = portfolio.position(order.instrument_id).quantity;
    const Quantity delta = order.side == Side::Buy ? order.quantity : -order.quantity;
    const Quantity projected = cur + delta;
    const Quantity abs_proj = projected >= 0 ? projected : -projected;
    if (abs_proj > limits_.max_abs_position) {
      return {false, "max_abs_position exceeded"};
    }
  }

  return {true, {}};
}

}  // namespace qflow
