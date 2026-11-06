#include <cstdlib>
#include <iostream>
#include <vector>

#include "qflow/engine.hpp"
#include "qflow/strategy.hpp"

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
  qflow::Portfolio book(5'000'000);
  qflow::BuyOnceStrategy strat(1, 10);
  strat.bind(&book, 0.01);

  qflow::EventEngine engine;
  engine.set_handler(&strat);
  engine.post(qflow::make_tick_event(qflow::Tick{100, 19000, 1, 1, qflow::Side::Buy}));
  engine.post(qflow::make_tick_event(qflow::Tick{200, 19100, 1, 1, qflow::Side::Buy}));
  engine.run();

  expect(book.open_orders().size() == 1, "one market order placed");
  expect(book.find_order(1)->type == qflow::OrderType::Market, "market order");
  expect(book.find_order(1)->quantity == 10, "qty 10");
  expect(book.find_order(1)->status == qflow::OrderStatus::Accepted, "still open");

  {
    qflow::Portfolio book2(5'000'000);
    qflow::DualEmaCrossStrategy strat2(1, 5, 2, 5);
    strat2.bind(&book2, 0.01);
    qflow::EventEngine engine2;
    engine2.set_handler(&strat2);
    for (int i = 0; i < 30; ++i) {
      const qflow::PriceTicks px = (i < 15) ? 10000 : 12000;
      engine2.post(qflow::make_tick_event(
          qflow::Tick{static_cast<qflow::TimestampNs>(i + 1), px, 1, 1,
                      qflow::Side::Buy}));
    }
    engine2.run();
    expect(!book2.open_orders().empty() || book2.position(1).quantity > 0 ||
               strat2.signal().ready(),
           "dual ema became ready / acted");
    expect(strat2.signal().ready(), "signal ready after warmup");
  }

  if (failures) {
    std::cerr << failures << " failed\n";
    return EXIT_FAILURE;
  }
  std::cout << "All strategy checks passed\n";
  return EXIT_SUCCESS;
}
