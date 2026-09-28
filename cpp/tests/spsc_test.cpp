#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>

#include "qflow/spsc_queue.hpp"
#include "qflow/tick.hpp"

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
    qflow::SpscQueue<int, 8> q;
    expect(q.try_push(1), "push1");
    expect(q.try_push(2), "push2");
    int v = 0;
    expect(q.try_pop(v) && v == 1, "pop1");
    expect(q.try_pop(v) && v == 2, "pop2");
    expect(!q.try_pop(v), "empty");
  }

  {
    qflow::SpscQueue<int, 4> q;
    expect(q.try_push(1), "a");
    expect(q.try_push(2), "b");
    expect(q.try_push(3), "c");
    expect(!q.try_push(4), "full (one slot reserved)");
  }

  {
    qflow::SpscQueue<qflow::Tick, 1024> q;
    constexpr int kN = 10000;
    std::thread producer([&] {
      for (int i = 1; i <= kN; ++i) {
        qflow::Tick t{i, i, 1, 1, qflow::Side::Buy};
        while (!q.try_push(t)) {
          std::this_thread::yield();
        }
      }
    });
    std::thread consumer([&] {
      int got = 0;
      qflow::Tick t{};
      while (got < kN) {
        if (q.try_pop(t)) {
          ++got;
          if (t.ts_ns != got) {
            std::cerr << "FAIL: order\n";
            ++failures;
            break;
          }
        } else {
          std::this_thread::yield();
        }
      }
    });
    producer.join();
    consumer.join();
    expect(failures == 0, "spsc threaded order ok");
  }

  if (failures) {
    std::cerr << failures << " failed\n";
    return EXIT_FAILURE;
  }
  std::cout << "All spsc checks passed\n";
  return EXIT_SUCCESS;
}
