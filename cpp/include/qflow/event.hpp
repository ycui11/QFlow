#pragma once

#include <cstdint>
#include <variant>

#include "qflow/bar.hpp"
#include "qflow/tick.hpp"
#include "qflow/top_of_book.hpp"
#include "qflow/types.hpp"

namespace qflow {

/** Timer fire with no market payload (used for scheduled wakeups). */
struct TimerEvent {
  std::uint32_t id{0};
};

using EventBody = std::variant<Tick, Bar, TopOfBook, TimerEvent>;

enum class EventType : std::uint8_t {
  Tick = 0,
  Bar = 1,
  TopOfBook = 2,
  Timer = 3,
};

/**
 * A single unit of work for the event engine.
 * Ordering key is (ts_ns, seq): time first, then insertion order.
 */
struct Event {
  TimestampNs ts_ns{0};
  std::uint64_t seq{0};
  EventBody body{};
};

inline EventType event_type(const Event& event) noexcept {
  return static_cast<EventType>(event.body.index());
}

inline Event make_tick_event(Tick tick, std::uint64_t seq = 0) {
  const TimestampNs ts = tick.ts_ns;
  return Event{ts, seq, EventBody{std::move(tick)}};
}

inline Event make_bar_event(Bar bar, std::uint64_t seq = 0) {
  const TimestampNs ts = bar.start_ts_ns;
  return Event{ts, seq, EventBody{std::move(bar)}};
}

inline Event make_top_of_book_event(TopOfBook book, std::uint64_t seq = 0) {
  const TimestampNs ts = book.ts_ns;
  return Event{ts, seq, EventBody{std::move(book)}};
}

inline Event make_timer_event(TimestampNs ts_ns, std::uint32_t timer_id,
                              std::uint64_t seq = 0) {
  return Event{ts_ns, seq, EventBody{TimerEvent{timer_id}}};
}

}  // namespace qflow
