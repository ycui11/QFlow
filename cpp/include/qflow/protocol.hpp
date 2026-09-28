#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "qflow/tick.hpp"

namespace qflow {

/**
 * Text wire format (one tick per line):
 *   T,<ts_ns>,<instrument_id>,<price_ticks>,<size>,<side>
 * side is BUY/SELL or 0/1.
 *
 * Framing is newline-delimited; binary/UDP binary packs can come later.
 */
bool parse_tick_line(std::string_view line, Tick* out,
                     std::string* error_message = nullptr);

std::string format_tick_line(const Tick& tick);

/**
 * Incremental parser for a byte stream (TCP-style).
 * Append chunks with feed(); drain complete ticks with pop().
 */
class TickLineParser {
 public:
  void reset();
  void feed(std::string_view chunk);
  bool pop(Tick* out);
  std::size_t buffered() const noexcept { return buffer_.size(); }
  const std::string& last_error() const noexcept { return last_error_; }

 private:
  std::string buffer_;
  std::string last_error_;
  std::vector<Tick> ready_;
  std::size_t ready_index_{0};
};

}  // namespace qflow
