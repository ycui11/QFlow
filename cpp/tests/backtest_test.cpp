#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "qflow/backtest.hpp"
#include "qflow/feed.hpp"
#include "qflow/instrument.hpp"

namespace {

int failures = 0;
void expect(bool c, const char* m) {
  if (!c) {
    std::cerr << "FAIL: " << m << '\n';
    ++failures;
  }
}

}  // namespace

int main() {
  std::vector<qflow::Tick> ticks{
      {100, 19000, 50, 1, qflow::Side::Buy},
      {200, 18950, 50, 1, qflow::Side::Sell},
      {300, 19100, 50, 1, qflow::Side::Buy},
  };

  qflow::Portfolio book(5'000'000);
  qflow::BuyOnceStrategy strat(1, 10);
  qflow::Backtester bt(book, strat, 0.01, 1);
  qflow::ReplayTickFeed feed(std::move(ticks));
  const qflow::BacktestResult result = bt.run(feed);

  expect(result.events_delivered == 3, "3 events");
  expect(result.fills == 1, "one fill from matcher");
  expect(result.final_position == 10, "long 10");
  expect(book.cash_cents() < 5'000'000, "cash spent");
  expect(bt.fills().size() == 1, "recorded fill");
  expect(bt.fills()[0].price == 19000, "filled at first trade");
  expect(result.final_equity_cents > 0, "equity reported");
  expect(std::isfinite(result.total_return), "total_return finite");
  expect(result.max_drawdown >= 0.0, "drawdown non-negative");

  // Limit order that should not fill when price is above limit on buys.
  {
    qflow::Portfolio book2(5'000'000);
    qflow::Order order{};
    order.instrument_id = 1;
    order.side = qflow::Side::Buy;
    order.type = qflow::OrderType::Limit;
    order.limit_price = 18000;
    order.quantity = 5;
    order.created_ts_ns = 1;
    book2.place(order);
    qflow::ImmediateMatcher matcher;
    qflow::Tick tick{1, 19000, 1, 1, qflow::Side::Buy};
    expect(matcher.match(book2, tick, 0.01) == 0, "limit not crossed");
  }

  // Slippage + commission on a market buy.
  {
    qflow::Portfolio book3(5'000'000);
    qflow::Order order{};
    order.instrument_id = 1;
    order.side = qflow::Side::Buy;
    order.type = qflow::OrderType::Market;
    order.quantity = 10;
    order.created_ts_ns = 1;
    book3.place(order);

    qflow::MatchCosts costs;
    costs.slippage_ticks = 2;
    costs.commission_cents = 50;
    qflow::ImmediateMatcher matcher(costs);
    qflow::Tick tick{1, 19000, 1, 1, qflow::Side::Buy};
    expect(matcher.match(book3, tick, 0.01) == 1, "market fill with costs");
    expect(book3.position(1).quantity == 10, "filled qty");
    // Fill at 19002 ticks = $190.02; 10 shares = $1900.20 + $0.50 fee
    expect(book3.cash_cents() == 5'000'000 - 190'020 - 50, "slip+fee cash");
  }

  // Dual EMA strategy produces at least one round trip on a step series.
  {
    std::vector<qflow::Tick> series;
    qflow::TimestampNs ts = 1;
    auto push = [&](double px) {
      series.push_back(
          {ts, qflow::to_ticks(px, 0.01), 10, 1, qflow::Side::Buy});
      ts += 1;
    };
    for (int i = 0; i < 40; ++i) {
      push(100.0);
    }
    for (int i = 0; i < 40; ++i) {
      push(120.0);
    }
    for (int i = 0; i < 40; ++i) {
      push(100.0);
    }

    qflow::Portfolio book4(5'000'000);
    qflow::DualEmaCrossStrategy strat4(1, 10, 3, 10);
    qflow::MatchCosts costs4;
    costs4.slippage_ticks = 1;
    costs4.commission_cents = 100;
    qflow::Backtester bt4(book4, strat4, 0.01, 1, costs4);
    qflow::ReplayTickFeed feed4(std::move(series));
    const qflow::BacktestResult r4 = bt4.run(feed4);
    expect(r4.fills >= 2, "dual ema bought and sold");
    expect(r4.closed_trades >= 1, "at least one closed trade");
    expect(r4.win_rate >= 0.0 && r4.win_rate <= 1.0, "win_rate in range");
  }

  if (failures) {
    std::cerr << failures << " failed\n";
    return EXIT_FAILURE;
  }
  std::cout << "All backtest checks passed\n";
  return EXIT_SUCCESS;
}
