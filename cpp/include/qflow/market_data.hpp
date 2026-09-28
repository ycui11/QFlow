#pragma once

#include <string>

#include "qflow/bar.hpp"
#include "qflow/instrument.hpp"
#include "qflow/tick.hpp"
#include "qflow/top_of_book.hpp"
#include "qflow/types.hpp"

namespace qflow {

/** Format helpers for debugging and CLI demos (not on the hot path). */
const char* to_string(Side side) noexcept;
std::string format(const Instrument& instrument);
std::string format(const Tick& tick, double tick_size);
std::string format(const Bar& bar, double tick_size);
std::string format(const TopOfBook& book, double tick_size);

}  // namespace qflow
