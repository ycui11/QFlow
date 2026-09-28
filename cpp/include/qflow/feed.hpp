#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "qflow/engine.hpp"
#include "qflow/tick.hpp"

namespace qflow {

/**
 * Source of market events. Feeds only post into an EventEngine;
 * they do not run strategies or advance the clock themselves.
 */
class MarketFeed {
 public:
  virtual ~MarketFeed() = default;

  /**
   * Post the next batch of events (possibly one) into the engine.
   * Returns how many events were posted. Zero means the feed is exhausted
   * for now (end of backtest file, or no new live data).
   */
  virtual std::size_t poll(EventEngine& engine) = 0;
};

/** Drain a feed until poll() returns 0. Returns total events posted. */
std::size_t pump_all(MarketFeed& feed, EventEngine& engine);

/** In-memory tick replay (unit tests and synthetic data). */
class ReplayTickFeed : public MarketFeed {
 public:
  explicit ReplayTickFeed(std::vector<Tick> ticks);

  std::size_t poll(EventEngine& engine) override;

  std::size_t size() const noexcept { return ticks_.size(); }
  std::size_t remaining() const noexcept {
    return next_ >= ticks_.size() ? 0 : ticks_.size() - next_;
  }

 private:
  std::vector<Tick> ticks_;
  std::size_t next_{0};
};

/**
 * CSV tick feed.
 *
 * Format (header required):
 *   ts_ns,instrument_id,price,size,side
 * price is decimal; converted with the feed's tick_size.
 * side is BUY/SELL (case-insensitive) or 0/1.
 */
class CsvTickFeed : public MarketFeed {
 public:
  CsvTickFeed() = default;

  /** Load ticks from path. On failure returns false and sets error_message. */
  bool load(const std::string& path, double tick_size, std::string* error_message = nullptr);

  std::size_t poll(EventEngine& engine) override;

  std::size_t size() const noexcept { return replay_.size(); }
  std::size_t remaining() const noexcept { return replay_.remaining(); }

 private:
  ReplayTickFeed replay_{{}};
};

}  // namespace qflow
