#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "qflow/feed.hpp"

namespace {

int failures = 0;

void expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    ++failures;
  }
}

class CountingHandler : public qflow::EventHandler {
 public:
  std::size_t count{0};
  std::vector<qflow::PriceTicks> prices;

  void on_event(const qflow::Event& event,
                const qflow::SimClock&) override {
    ++count;
    if (const auto* tick = std::get_if<qflow::Tick>(&event.body)) {
      prices.push_back(tick->price);
    }
  }
};

}  // namespace

int main(int argc, char** argv) {
  {
    std::vector<qflow::Tick> ticks{
        {300, 30, 1, 1, qflow::Side::Buy},
        {100, 10, 1, 1, qflow::Side::Sell},
        {200, 20, 1, 1, qflow::Side::Buy},
    };
    CountingHandler handler;
    qflow::EventEngine engine;
    engine.set_handler(&handler);

    qflow::ReplayTickFeed feed(std::move(ticks));
    expect(qflow::pump_all(feed, engine) == 3, "replay posts 3");
    expect(feed.poll(engine) == 0, "exhausted replay");
    expect(engine.run() == 3, "engine delivers 3");
    expect(handler.prices.size() == 3, "three prices");
    expect(handler.prices[0] == 10 && handler.prices[1] == 20 &&
               handler.prices[2] == 30,
           "time-ordered prices");
  }

  if (argc < 2) {
    std::cerr << "usage: qflow_feed_test <sample_ticks.csv>\n";
    return EXIT_FAILURE;
  }

  {
    const std::string path = argv[1];
    qflow::CsvTickFeed feed;
    std::string err;
    expect(feed.load(path, 0.01, &err), "csv load ok");
    if (!err.empty() && failures) {
      std::cerr << "csv error: " << err << '\n';
    }
    expect(feed.size() == 4, "sample has 4 ticks");

    CountingHandler handler;
    qflow::EventEngine engine;
    engine.set_handler(&handler);
    expect(qflow::pump_all(feed, engine) == 4, "csv pump 4");
    expect(engine.run() == 4, "csv deliver 4");
    expect(handler.prices.front() == 19000, "first price 190.00");
    expect(handler.prices.back() == 19010, "last price 190.10");
  }

  {
    qflow::CsvTickFeed feed;
    std::string err;
    expect(!feed.load("/no/such/file.csv", 0.01, &err), "missing file fails");
    expect(!err.empty(), "error message set");
  }

  if (failures != 0) {
    std::cerr << failures << " test(s) failed\n";
    return EXIT_FAILURE;
  }
  std::cout << "All feed checks passed\n";
  return EXIT_SUCCESS;
}
