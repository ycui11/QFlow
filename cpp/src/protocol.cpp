#include "qflow/protocol.hpp"

#include <cctype>
#include <sstream>

namespace qflow {
namespace {

std::string_view trim_view(std::string_view s) {
  while (!s.empty() &&
         std::isspace(static_cast<unsigned char>(s.front()))) {
    s.remove_prefix(1);
  }
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
    s.remove_suffix(1);
  }
  return s;
}

bool parse_side_token(std::string_view raw, Side* side) {
  std::string s(trim_view(raw));
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

bool parse_tick_line(std::string_view line, Tick* out, std::string* error_message) {
  line = trim_view(line);
  if (line.empty() || line[0] == '#') {
    if (error_message != nullptr) {
      *error_message = "empty or comment";
    }
    return false;
  }

  std::string copy(line);
  std::stringstream ss(copy);
  std::string tag;
  std::string ts_s;
  std::string id_s;
  std::string px_s;
  std::string sz_s;
  std::string side_s;
  if (!std::getline(ss, tag, ',') || !std::getline(ss, ts_s, ',') ||
      !std::getline(ss, id_s, ',') || !std::getline(ss, px_s, ',') ||
      !std::getline(ss, sz_s, ',') || !std::getline(ss, side_s, ',')) {
    if (error_message != nullptr) {
      *error_message = "expected T,ts,id,price_ticks,size,side";
    }
    return false;
  }
  tag = std::string(trim_view(tag));
  if (tag != "T") {
    if (error_message != nullptr) {
      *error_message = "unsupported tag (want T)";
    }
    return false;
  }

  Tick tick{};
  try {
    tick.ts_ns = std::stoll(std::string(trim_view(ts_s)));
    tick.instrument_id = static_cast<InstrumentId>(std::stoul(std::string(trim_view(id_s))));
    tick.price = std::stoll(std::string(trim_view(px_s)));
    tick.size = std::stoll(std::string(trim_view(sz_s)));
  } catch (const std::exception&) {
    if (error_message != nullptr) {
      *error_message = "numeric parse failed";
    }
    return false;
  }
  if (!parse_side_token(side_s, &tick.side)) {
    if (error_message != nullptr) {
      *error_message = "bad side";
    }
    return false;
  }
  if (!is_valid(tick)) {
    if (error_message != nullptr) {
      *error_message = "invalid tick fields";
    }
    return false;
  }
  *out = tick;
  return true;
}

std::string format_tick_line(const Tick& tick) {
  std::ostringstream out;
  out << "T," << tick.ts_ns << ',' << tick.instrument_id << ',' << tick.price
      << ',' << tick.size << ',' << (tick.side == Side::Buy ? "BUY" : "SELL");
  return out.str();
}

void TickLineParser::reset() {
  buffer_.clear();
  last_error_.clear();
  ready_.clear();
  ready_index_ = 0;
}

void TickLineParser::feed(std::string_view chunk) {
  buffer_.append(chunk.data(), chunk.size());
  while (true) {
    const auto pos = buffer_.find('\n');
    if (pos == std::string::npos) {
      break;
    }
    std::string_view line(buffer_.data(), pos);
    if (!line.empty() && line.back() == '\r') {
      line.remove_suffix(1);
    }
    Tick tick{};
    std::string err;
    if (parse_tick_line(line, &tick, &err)) {
      ready_.push_back(tick);
    } else if (!line.empty() && line[0] != '#') {
      last_error_ = err;
    }
    buffer_.erase(0, pos + 1);
  }
}

bool TickLineParser::pop(Tick* out) {
  if (ready_index_ >= ready_.size()) {
    return false;
  }
  *out = ready_[ready_index_++];
  if (ready_index_ == ready_.size()) {
    ready_.clear();
    ready_index_ = 0;
  }
  return true;
}

}  // namespace qflow
