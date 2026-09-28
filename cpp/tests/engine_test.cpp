#include <cstdlib>
#include <iostream>
#include <vector>

#include "qflow/engine.hpp"

namespace {

int failures = 0;

void expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    ++failures;
  }
}

class RecordingHandler : public qflow::EventHandler {
 public:
  std::vector<qflow::TimestampNs> times;
  std::vector<qflow::EventType> types;
  std::vector<std::uint64_t> seqs;
  std::vector<qflow::TimestampNs> clock_now;

  void on_event(const qflow::Event& event,
                const qflow::SimClock& clock) override {
    times.push_back(event.ts_ns);
    types.push_back(qflow::event_type(event));
    seqs.push_back(event.seq);
    clock_now.push_back(clock.now());
  }
};

}  // namespace

int main() {
  {
    qflow::EventEngine engine;
    expect(engine.run() == 0, "empty run delivers nothing");
    expect(engine.clock().now() == 0, "clock starts at 0");
  }

  {
    RecordingHandler handler;
    qflow::EventEngine engine;
    engine.set_handler(&handler);

    // Post out of order; engine must deliver by timestamp.
    engine.post(qflow::make_tick_event(qflow::Tick{300, 10, 1, 1, qflow::Side::Buy}));
    engine.post(qflow::make_tick_event(qflow::Tick{100, 11, 1, 1, qflow::Side::Sell}));
    engine.post(qflow::make_bar_event(qflow::Bar{200, 250, 1, 1, 2, 1, 2, 5}));

    expect(engine.pending() == 3, "three events queued");
    expect(engine.run() == 3, "three events delivered");
    expect(engine.empty(), "queue drained");

    expect(handler.times.size() == 3, "recorded three times");
    expect(handler.times[0] == 100 && handler.times[1] == 200 &&
               handler.times[2] == 300,
           "delivered in time order");
    expect(handler.types[0] == qflow::EventType::Tick, "first is tick");
    expect(handler.types[1] == qflow::EventType::Bar, "second is bar");
    expect(handler.types[2] == qflow::EventType::Tick, "third is tick");
    expect(handler.clock_now[0] == 100 && handler.clock_now[2] == 300,
           "clock tracks event time");
  }

  {
    RecordingHandler handler;
    qflow::EventEngine engine;
    engine.set_handler(&handler);

    // Same timestamp: lower seq first (post order when seq auto-assigned).
    engine.post(qflow::make_timer_event(50, 1));
    engine.post(qflow::make_timer_event(50, 2));
    engine.run();

    expect(handler.seqs.size() == 2, "two timers");
    expect(handler.seqs[0] < handler.seqs[1], "seq order at same timestamp");
    expect(handler.types[0] == qflow::EventType::Timer, "timer type");
  }

  {
    qflow::SimClock clock;
    expect(clock.advance_to(10), "advance forward");
    expect(!clock.advance_to(5), "reject past");
    expect(clock.now() == 10, "clock unchanged on reject");
  }

  if (failures != 0) {
    std::cerr << failures << " test(s) failed\n";
    return EXIT_FAILURE;
  }
  std::cout << "All engine checks passed\n";
  return EXIT_SUCCESS;
}
