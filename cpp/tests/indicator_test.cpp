#include <cmath>
#include <cstdlib>
#include <iostream>

#include "qflow/indicator.hpp"

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
    qflow::Sma sma(3);
    sma.update(1);
    sma.update(2);
    expect(!sma.ready(), "sma not ready at 2");
    sma.update(3);
    expect(sma.ready(), "sma ready");
    expect(std::abs(sma.value() - 2.0) < 1e-9, "sma=2");
    sma.update(6);
    expect(std::abs(sma.value() - 3.6666666667) < 1e-6 ||
               std::abs(sma.value() - (2.0 + 3.0 + 6.0) / 3.0) < 1e-9,
           "sma rolls");
  }

  {
    qflow::Ema ema(3);
    ema.update(10);
    expect(ema.ready() && ema.value() == 10, "ema seeds");
    ema.update(20);
    expect(ema.value() > 10 && ema.value() < 20, "ema between");
  }

  {
    qflow::EmaCrossSignal sig(2, 4);
    // Drive a clear up-cross then down-cross with a step series.
    for (double p : {1, 1, 1, 1, 1, 10, 10, 10, 10, 1, 1, 1, 1}) {
      sig.update(p);
    }
    // We only assert the API is usable and becomes ready; cross flags are
    // path-dependent — check ready and that values are finite.
    expect(sig.ready(), "cross ready");
    expect(std::isfinite(sig.fast()) && std::isfinite(sig.slow()), "finite");
  }

  {
    // Deterministic cross-up: slow starts higher conceptually after flat then jump.
    qflow::EmaCrossSignal sig(2, 5);
    bool saw_up = false;
    for (int i = 0; i < 20; ++i) {
      sig.update(1.0);
    }
    for (int i = 0; i < 20; ++i) {
      sig.update(100.0);
      if (sig.crossed_up()) {
        saw_up = true;
      }
    }
    expect(saw_up, "saw bullish cross");
  }

  if (failures) {
    std::cerr << failures << " failed\n";
    return EXIT_FAILURE;
  }
  std::cout << "All indicator checks passed\n";
  return EXIT_SUCCESS;
}
