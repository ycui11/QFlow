#include <cstdlib>
#include <iostream>
#include <vector>

#include "qflow/layout.hpp"

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
  expect(sizeof(qflow::Tick) == 32, "Tick is 32 bytes after packing");
  expect(alignof(qflow::Tick) == 8, "Tick align is 8");
  expect(sizeof(qflow::Bar) == 64, "Bar is one cache line");
  expect(sizeof(qflow::TopOfBook) <= qflow::kCacheLineBytes, "BBO fits in a line");

  expect(qflow::is_hot_path_type_v<qflow::Tick>, "Tick is a hot-path type");
  expect(offsetof(qflow::Tick, ts_ns) == 0, "ts_ns at offset 0");
  expect(offsetof(qflow::Tick, price) == 8, "price at offset 8");
  expect(offsetof(qflow::Tick, size) == 16, "size at offset 16");
  expect(offsetof(qflow::Tick, instrument_id) == 24, "instrument_id at offset 24");
  expect(offsetof(qflow::Tick, side) == 28, "side at offset 28");

  std::vector<qflow::Tick> ticks(4);
  for (std::size_t i = 0; i < ticks.size(); ++i) {
    ticks[i].price = static_cast<qflow::PriceTicks>(100 + i);
  }
  expect(qflow::sum_prices(ticks.data(), ticks.size()) == 406, "AoS price sum");

  // Passing by HotRef must not require a copy of the referent's identity.
  const qflow::Tick sample{1, 19025, 10, 1, qflow::Side::Buy};
  qflow::HotRef<qflow::Tick> ref = sample;
  expect(&ref == &sample, "HotRef binds to the same object");

  if (failures != 0) {
    std::cerr << failures << " test(s) failed\n";
    return EXIT_FAILURE;
  }
  std::cout << "All layout checks passed\n";
  return EXIT_SUCCESS;
}
