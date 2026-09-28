#include "qflow/strategy.hpp"

#include <type_traits>
#include <variant>

namespace qflow {

void Strategy::on_event(const Event& event, const SimClock& clock) {
  if (portfolio_ == nullptr) {
    return;
  }
  StrategyContext ctx(*portfolio_, tick_size_, clock, gateway_);
  std::visit(
      [&](const auto& body) {
        using T = std::decay_t<decltype(body)>;
        if constexpr (std::is_same_v<T, Tick>) {
          on_tick(body, ctx);
        } else if constexpr (std::is_same_v<T, Bar>) {
          on_bar(body, ctx);
        } else if constexpr (std::is_same_v<T, TopOfBook>) {
          on_top_of_book(body, ctx);
        } else if constexpr (std::is_same_v<T, TimerEvent>) {
          on_timer(body, ctx);
        }
      },
      event.body);
}

void BuyOnceStrategy::on_tick(const Tick& tick, StrategyContext& ctx) {
  if (done_ || tick.instrument_id != instrument_id_) {
    return;
  }
  ctx.place_market(instrument_id_, Side::Buy, quantity_, nullptr, tick.price);
  done_ = true;
}

}  // namespace qflow
