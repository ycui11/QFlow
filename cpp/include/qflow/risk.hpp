#pragma once

#include <cstddef>
#include <string>

#include "qflow/order.hpp"
#include "qflow/portfolio.hpp"

namespace qflow {

/** Hard limits checked before any order reaches the book / venue. */
struct RiskLimits {
  /** Max shares/contracts per order; 0 disables this check. */
  Quantity max_order_qty{0};
  /** Max absolute net position after a hypothetical fill; 0 disables. */
  Quantity max_abs_position{0};
  /** Max order notional in cents; 0 disables. */
  MoneyCents max_order_notional_cents{0};
  /** Max accepted orders in this session; 0 disables. */
  std::size_t max_orders_per_session{0};
};

struct RiskDecision {
  bool allowed{false};
  std::string reason;
};

/**
 * Pre-trade risk gate. Call evaluate() before Portfolio::place / venue send.
 * kill() flips trading_enabled off (manual or automated halt).
 */
class RiskGate {
 public:
  explicit RiskGate(RiskLimits limits = {});

  const RiskLimits& limits() const noexcept { return limits_; }
  void set_limits(RiskLimits limits) { limits_ = limits; }

  bool trading_enabled() const noexcept { return trading_enabled_; }
  void set_trading_enabled(bool enabled) noexcept { trading_enabled_ = enabled; }
  void kill() noexcept { trading_enabled_ = false; }

  std::size_t accepted_count() const noexcept { return accepted_count_; }

  RiskDecision evaluate(const Portfolio& portfolio, const Order& order,
                        double tick_size) const;

  /** Call after an order is successfully accepted by the gateway. */
  void on_accept() noexcept { ++accepted_count_; }

 private:
  RiskLimits limits_{};
  bool trading_enabled_{true};
  std::size_t accepted_count_{0};
};

}  // namespace qflow
