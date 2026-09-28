#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "qflow/feed.hpp"
#include "qflow/fill.hpp"
#include "qflow/portfolio.hpp"
#include "qflow/strategy.hpp"
#include "qflow/tick.hpp"

namespace qflow {

/**
 * Naive immediate matcher for backtests:
 * - Market orders fill at the trade price.
 * - Limit buys fill when trade price <= limit; limit sells when >= limit.
 * Processes open orders in id order on each tick.
 */
class ImmediateMatcher {
 public:
  std::size_t match(Portfolio& portfolio, const Tick& tick, double tick_size,
                    std::vector<Fill>* fills_out = nullptr);
};

struct BacktestResult {
  std::size_t events_delivered{0};
  std::size_t fills{0};
  MoneyCents final_cash_cents{0};
  Quantity final_position{0};
  MoneyCents realized_pnl_cents{0};  // final_cash - initial_cash (ignores open MTM)
};

/**
 * Runs feed → engine → strategy, matching open orders after each tick.
 */
class Backtester {
 public:
  Backtester(Portfolio& portfolio, Strategy& strategy, double tick_size,
             InstrumentId primary_instrument = 1);

  BacktestResult run(MarketFeed& feed);

  const std::vector<Fill>& fills() const noexcept { return fills_; }

 private:
  class Session;

  Portfolio& portfolio_;
  Strategy& strategy_;
  double tick_size_;
  InstrumentId primary_instrument_;
  ImmediateMatcher matcher_{};
  std::vector<Fill> fills_{};
  MoneyCents initial_cash_{0};
};

}  // namespace qflow
