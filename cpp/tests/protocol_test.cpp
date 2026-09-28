#include <cstdlib>
#include <iostream>
#include <string>

#include "qflow/protocol.hpp"

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
  {
    qflow::Tick tick{};
    std::string err;
    expect(qflow::parse_tick_line("T,100,1,19025,10,BUY", &tick, &err), "parse");
    expect(tick.ts_ns == 100 && tick.price == 19025 && tick.size == 10, "fields");
    expect(tick.side == qflow::Side::Buy, "side");
    const std::string line = qflow::format_tick_line(tick);
    qflow::Tick again{};
    expect(qflow::parse_tick_line(line, &again, &err), "roundtrip");
    expect(again.price == tick.price, "roundtrip price");
  }

  {
    qflow::TickLineParser parser;
    parser.feed("T,1,1,100,1,BUY\nT,2,1,101,2,SELL");
    expect(parser.buffered() > 0, "partial line buffered");
    qflow::Tick t{};
    expect(parser.pop(&t) && t.ts_ns == 1, "first complete");
    expect(!parser.pop(&t), "second incomplete");
    parser.feed("\n");
    expect(parser.pop(&t) && t.ts_ns == 2 && t.side == qflow::Side::Sell,
           "second after newline");
  }

  {
    qflow::Tick tick{};
    std::string err;
    expect(!qflow::parse_tick_line("X,1,1,1,1,BUY", &tick, &err), "bad tag");
  }

  if (failures) {
    std::cerr << failures << " failed\n";
    return EXIT_FAILURE;
  }
  std::cout << "All protocol checks passed\n";
  return EXIT_SUCCESS;
}
