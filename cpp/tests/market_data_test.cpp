#include <cstdlib>
#include <iostream>

#include "qflow/market_data.hpp"

namespace {

int failures = 0;

void expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    ++failures;
  }
}

}  // namespace

int main() {
  const qflow::Instrument aapl{1, "AAPL", 0.01};
  expect(qflow::is_valid(aapl), "valid instrument");

  const qflow::PriceTicks px = qflow::to_ticks(190.25, aapl.tick_size);
  expect(px == 19025, "190.25 maps to 19025 ticks at 0.01");
  expect(qflow::to_price(px, aapl.tick_size) == 190.25, "ticks convert back");

  qflow::Tick tick{1, px, 10, aapl.id, qflow::Side::Sell};
  expect(qflow::is_valid(tick), "valid tick");
  tick.size = 0;
  expect(!qflow::is_valid(tick), "zero size tick is invalid");

  qflow::Bar bar{1, 2, aapl.id, 100, 110, 90, 105, 1000};
  expect(qflow::is_valid(bar), "valid bar");
  bar.high = 80;
  expect(!qflow::is_valid(bar), "high < low is invalid");

  qflow::TopOfBook book{1, aapl.id, 100, 5, 101, 7};
  expect(qflow::is_valid(book), "valid top of book");
  expect(qflow::spread_ticks(book) == 1, "spread is 1 tick");
  expect(qflow::mid_ticks(book) == 100, "integer mid truncates toward zero");

  if (failures != 0) {
    std::cerr << failures << " test(s) failed\n";
    return EXIT_FAILURE;
  }
  std::cout << "All market data checks passed\n";
  return EXIT_SUCCESS;
}
