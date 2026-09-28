# QFlow

QFlow is an independent open-source quantitative / low-latency trading system
built from scratch in **C++** (hot path) with a **Python** research layer (cold path).

```text
market data → event engine → strategy → matcher/portfolio → (optional) Python research
```

## Build

Requires CMake 3.20+, a C++17 compiler, and (for Python bindings) Python 3 development
headers. The first configure with Python enabled downloads pybind11 via FetchContent.

```bash
bash build.sh                 # configure + build + ctest
bash build.sh --run           # also run ./build/qflow_cli
cmake -S . -B build -DQFLOW_BUILD_PYTHON=OFF   # C++ only
```

## Layout

| Path | Purpose |
|------|---------|
| `cpp/include/qflow/` | Public C++ headers |
| `cpp/src/` | Core implementation |
| `cpp/tests/` | C++ unit tests (ctest) |
| `cpp/python/` | pybind11 module sources |
| `python/examples/` | Python smoke scripts |
| `data/` | Sample CSV ticks |

## Core modules

| Module | Headers / notes |
|--------|-----------------|
| Market data | `tick.hpp`, `bar.hpp`, `top_of_book.hpp`, `layout.hpp` |
| Engine / feed | `engine.hpp`, `feed.hpp` |
| Account | `order.hpp`, `portfolio.hpp` |
| Strategy / backtest | `strategy.hpp`, `backtest.hpp` |
| Indicators | `indicator.hpp` |
| Concurrency | `spsc_queue.hpp` |
| Wire protocol | `protocol.hpp` (`T,ts,id,px,size,side`) |
| Risk / gateway | `risk.hpp`, `gateway.hpp` (sim vs paper/live boundary) |
| Python | import `qflow` after build (`PYTHONPATH=build`) |

Lesson notes and personal study plans are gitignored and not part of the public tree.

## License

TBD (will be set before the first public release).
