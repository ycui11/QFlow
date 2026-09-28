#include "qflow/feed.hpp"

#include <cctype>
#include <fstream>
#include <sstream>
#include <utility>

#include "qflow/instrument.hpp"

namespace qflow {
namespace {

std::string trim(std::string s) {
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
    s.erase(s.begin());
  }
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
    s.pop_back();
  }
  return s;
}

bool parse_side(const std::string& raw, Side* side) {
  std::string s = trim(raw);
  for (char& c : s) {
    c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  }
  if (s == "BUY" || s == "B" || s == "0") {
    *side = Side::Buy;
    return true;
  }
  if (s == "SELL" || s == "S" || s == "1") {
    *side = Side::Sell;
    return true;
  }
  return false;
}

}  // namespace

std::size_t pump_all(MarketFeed& feed, EventEngine& engine) {
  std::size_t total = 0;
  while (true) {
    const std::size_t n = feed.poll(engine);
    if (n == 0) {
      break;
    }
    total += n;
  }
  return total;
}

ReplayTickFeed::ReplayTickFeed(std::vector<Tick> ticks) : ticks_(std::move(ticks)) {}

std::size_t ReplayTickFeed::poll(EventEngine& engine) {
  if (next_ >= ticks_.size()) {
    return 0;
  }
  engine.post(make_tick_event(ticks_[next_]));
  ++next_;
  return 1;
}

bool CsvTickFeed::load(const std::string& path, double tick_size,
                       std::string* error_message) {
  if (tick_size <= 0.0) {
    if (error_message != nullptr) {
      *error_message = "tick_size must be positive";
    }
    return false;
  }

  std::ifstream in(path);
  if (!in) {
    if (error_message != nullptr) {
      *error_message = "failed to open file: " + path;
    }
    return false;
  }

  std::string line;
  if (!std::getline(in, line)) {
    if (error_message != nullptr) {
      *error_message = "empty CSV file";
    }
    return false;
  }

  std::vector<Tick> ticks;
  std::size_t line_no = 1;
  while (std::getline(in, line)) {
    ++line_no;
    line = trim(line);
    if (line.empty() || line[0] == '#') {
      continue;
    }

    std::stringstream row(line);
    std::string ts_s;
    std::string id_s;
    std::string price_s;
    std::string size_s;
    std::string side_s;
    if (!std::getline(row, ts_s, ',') || !std::getline(row, id_s, ',') ||
        !std::getline(row, price_s, ',') || !std::getline(row, size_s, ',') ||
        !std::getline(row, side_s, ',')) {
      if (error_message != nullptr) {
        *error_message = "bad CSV row at line " + std::to_string(line_no);
      }
      return false;
    }

    Tick tick{};
    try {
      tick.ts_ns = std::stoll(trim(ts_s));
      tick.instrument_id = static_cast<InstrumentId>(std::stoul(trim(id_s)));
      const double price = std::stod(trim(price_s));
      tick.price = to_ticks(price, tick_size);
      tick.size = std::stoll(trim(size_s));
    } catch (const std::exception&) {
      if (error_message != nullptr) {
        *error_message = "parse error at line " + std::to_string(line_no);
      }
      return false;
    }

    if (!parse_side(side_s, &tick.side)) {
      if (error_message != nullptr) {
        *error_message = "bad side at line " + std::to_string(line_no);
      }
      return false;
    }

    if (!is_valid(tick)) {
      if (error_message != nullptr) {
        *error_message = "invalid tick at line " + std::to_string(line_no);
      }
      return false;
    }
    ticks.push_back(tick);
  }

  replay_ = ReplayTickFeed(std::move(ticks));
  return true;
}

std::size_t CsvTickFeed::poll(EventEngine& engine) { return replay_.poll(engine); }

}  // namespace qflow
