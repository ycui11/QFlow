#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "qflow/bar.hpp"
#include "qflow/tick.hpp"
#include "qflow/top_of_book.hpp"

namespace qflow {

/** Typical x86-64 / Apple Silicon cache line size used for layout reasoning. */
inline constexpr std::size_t kCacheLineBytes = 64;

template <typename T>
inline constexpr bool is_hot_path_type_v =
    std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>;

/**
 * Prefer passing hot-path records by const reference when the call cannot be
 * fully inlined, or when the type may grow. Tiny trivially-copyable types may
 * still be passed by value; measure before micro-optimizing calling convention.
 */
template <typename T>
using HotRef = const T&;

// Compile-time layout contracts for core market-data records.
static_assert(is_hot_path_type_v<Tick>, "Tick must stay trivially copyable");
static_assert(is_hot_path_type_v<Bar>, "Bar must stay trivially copyable");
static_assert(is_hot_path_type_v<TopOfBook>, "TopOfBook must stay trivially copyable");

static_assert(sizeof(Tick) == 32, "Tick size drifted; revisit field order/padding");
static_assert(alignof(Tick) == 8, "Tick alignment drifted");
static_assert(sizeof(Tick) <= kCacheLineBytes, "Tick should fit in one cache line");

static_assert(sizeof(Bar) == 64, "Bar size drifted; revisit field order/padding");
static_assert(sizeof(Bar) == kCacheLineBytes, "Bar is intentionally one cache line");

static_assert(sizeof(TopOfBook) == 48, "TopOfBook size drifted");
static_assert(sizeof(TopOfBook) <= kCacheLineBytes, "TopOfBook should fit in one cache line");

/** Sum trade prices over a contiguous Tick array (cache-friendly AoS scan). */
inline std::int64_t sum_prices(const Tick* ticks, std::size_t count) noexcept {
  std::int64_t total = 0;
  for (std::size_t i = 0; i < count; ++i) {
    total += ticks[i].price;
  }
  return total;
}

}  // namespace qflow
