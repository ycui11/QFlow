#include <cstdlib>
#include <iostream>
#include <vector>

#include "qflow/backtest.hpp"
#include "qflow/feed.hpp"

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

  if (failures) {
    std::cerr << failures << " failed\n";
    return EXIT_FAILURE;
  }
  std::cout << "All backtest checks passed\n";
  return EXIT_SUCCESS;
}
