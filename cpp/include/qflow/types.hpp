#pragma once

#include <cstdint>

namespace qflow {

/** Nanoseconds since Unix epoch (UTC). */
using TimestampNs = std::int64_t;

/**
 * Price in integer tick units for an instrument.
 * Convert to decimal with: ticks * instrument.tick_size.
 * Integer prices avoid binary floating-point surprises on hot paths.
 */
using PriceTicks = std::int64_t;

/** Size in the instrument's quantity unit (shares, contracts, lots, ...). */
using Quantity = std::int64_t;

/** Stable numeric id for an instrument within a process / session. */
using InstrumentId = std::uint32_t;

enum class Side : std::uint8_t {
  Buy = 0,
  Sell = 1,
};

}  // namespace qflow
