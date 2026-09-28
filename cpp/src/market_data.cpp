#include "qflow/market_data.hpp"

#include <sstream>
#include <string>

namespace qflow {

const char* to_string(Side side) noexcept {
  return side == Side::Buy ? "BUY" : "SELL";
}

std::string format(const Instrument& instrument) {
  std::ostringstream out;
  out << "Instrument{id=" << instrument.id << ", symbol=" << instrument.symbol
      << ", tick_size=" << instrument.tick_size << '}';
  return out.str();
}

std::string format(const Tick& tick, double tick_size) {
  std::ostringstream out;
  out << "Tick{ts_ns=" << tick.ts_ns << ", instrument_id=" << tick.instrument_id
      << ", price=" << to_price(tick.price, tick_size) << ", size=" << tick.size
      << ", side=" << to_string(tick.side) << '}';
  return out.str();
}

std::string format(const Bar& bar, double tick_size) {
  std::ostringstream out;
  out << "Bar{start=" << bar.start_ts_ns << ", end=" << bar.end_ts_ns
      << ", instrument_id=" << bar.instrument_id
      << ", o=" << to_price(bar.open, tick_size)
      << ", h=" << to_price(bar.high, tick_size)
      << ", l=" << to_price(bar.low, tick_size)
      << ", c=" << to_price(bar.close, tick_size) << ", volume=" << bar.volume
      << '}';
  return out.str();
}

std::string format(const TopOfBook& book, double tick_size) {
  std::ostringstream out;
  out << "TopOfBook{ts_ns=" << book.ts_ns
      << ", instrument_id=" << book.instrument_id
      << ", bid=" << to_price(book.bid_price, tick_size) << 'x' << book.bid_size
      << ", ask=" << to_price(book.ask_price, tick_size) << 'x' << book.ask_size
      << ", mid=" << to_price(mid_ticks(book), tick_size)
      << ", spread_ticks=" << spread_ticks(book) << '}';
  return out.str();
}

}  // namespace qflow
