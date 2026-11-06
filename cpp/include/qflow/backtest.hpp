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

/** Simple execution costs applied by ImmediateMatcher. */
struct MatchCosts {
  /** Adverse ticks: buy fills higher, sell fills lower. */
  PriceTicks slippage_ticks{0};
  /** Fixed cash fee charged on every fill (cents). */
  MoneyCents commission_cents{0};
};

/**
 * Naive immediate matcher for backtests:
 * - Market orders fill at the (slipped) trade price.
 * - Limit buys fill when trade price <= limit; limit sells when >= limit.
 * Processes open orders in id order on each tick.
 */
class ImmediateMatcher {
 public:
  ImmediateMatcher() = default;
  explicit ImmediateMatcher(MatchCosts costs) : costs_(costs) {}

  void set_costs(MatchCosts costs) noexcept { costs_ = costs; }
  const MatchCosts& costs() const noexcept { return costs_; }

  std::size_t match(Portfolio& portfolio, const Tick& tick, double tick_size,
                    std::vector<Fill>* fills_out = nullptr);

 private:
  MatchCosts costs_{};
};

struct BacktestResult {
  std::size_t events_delivered{0};
  std::size_t fills{0};
  MoneyCents final_cash_cents{0};
  Quantity final_position{0};
  /** final_cash - initial_cash (open MTM not included here). */
  MoneyCents realized_pnl_cents{0};
  /** Mark-to-market equity at the last tick: cash + position * last_price. */
  MoneyCents final_equity_cents{0};
  /** (final_equity - initial_cash) / initial_cash. 0 if no initial cash. */
  double total_return{0.0};
  /** Max peak-to-trough drawdown on MTM equity, as a fraction of peak (0..1). */
  double max_drawdown{0.0};
  /** Closed round-trips (flat → position → flat). */
  std::size_t closed_trades{0};
  /** Fraction of closed trades with positive price PnL (before fees attribution). */
  double win_rate{0.0};
};

/**
 * Runs feed → engine → strategy, matching open orders after each tick.
 */
class Backtester {
 public:
  Backtester(Portfolio& portfolio, Strategy& strategy, double tick_size,
             InstrumentId primary_instrument = 1,
             MatchCosts costs = MatchCosts{});

  BacktestResult run(MarketFeed& feed);

  const std::vector<Fill>& fills() const noexcept { return fills_; }

 private:
  class Session;

  Portfolio& portfolio_;
  Strategy& strategy_;
  double tick_size_;
  InstrumentId primary_instrument_;
  ImmediateMatcher matcher_;
  std::vector<Fill> fills_{};
  MoneyCents initial_cash_{0};
};

}  // namespace qflow
