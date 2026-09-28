#include "qflow/backtest.hpp"

#include <algorithm>

#include "qflow/engine.hpp"

namespace qflow {
namespace {

bool limit_crosses(const Order& order, PriceTicks trade_price) noexcept {
  if (order.type == OrderType::Market) {
    return true;
  }
  if (order.side == Side::Buy) {
    return trade_price <= order.limit_price;
  }
  return trade_price >= order.limit_price;
}

}  // namespace

std::size_t ImmediateMatcher::match(Portfolio& portfolio, const Tick& tick,
                                    double tick_size,
                                    std::vector<Fill>* fills_out) {
  auto open = portfolio.open_orders();
  std::sort(open.begin(), open.end());

  std::size_t count = 0;
  for (OrderId id : open) {
    Order* order = portfolio.find_order(id);
    if (order == nullptr || !is_open(*order)) {
      continue;
    }
    if (order->instrument_id != tick.instrument_id) {
      continue;
    }
    if (!limit_crosses(*order, tick.price)) {
      continue;
    }

    const Quantity qty = leaves(*order);
    if (qty <= 0) {
      continue;
    }

    Fill fill{id, tick.instrument_id, tick.ts_ns, order->side, tick.price, qty};
    std::string err;
    if (!portfolio.apply_fill(fill, tick_size, &err)) {
      continue;
    }
    if (fills_out != nullptr) {
      fills_out->push_back(fill);
    }
    ++count;
  }
  return count;
}

class Backtester::Session : public EventHandler {
 public:
  Session(Backtester& owner) : owner_(owner) {}

  void on_event(const Event& event, const SimClock& clock) override {
    owner_.strategy_.on_event(event, clock);
    if (const auto* tick = std::get_if<Tick>(&event.body)) {
      owner_.matcher_.match(owner_.portfolio_, *tick, owner_.tick_size_,
                            &owner_.fills_);
    }
  }

 private:
  Backtester& owner_;
};

Backtester::Backtester(Portfolio& portfolio, Strategy& strategy, double tick_size,
                       InstrumentId primary_instrument)
    : portfolio_(portfolio),
      strategy_(strategy),
      tick_size_(tick_size),
      primary_instrument_(primary_instrument),
      initial_cash_(portfolio.cash_cents()) {
  strategy_.bind(&portfolio_, tick_size_);
  (void)primary_instrument_;
}

BacktestResult Backtester::run(MarketFeed& feed) {
  fills_.clear();
  initial_cash_ = portfolio_.cash_cents();

  Session session(*this);
  EventEngine engine;
  engine.set_handler(&session);
  pump_all(feed, engine);
  const std::size_t delivered = engine.run();

  BacktestResult result;
  result.events_delivered = delivered;
  result.fills = fills_.size();
  result.final_cash_cents = portfolio_.cash_cents();
  result.final_position = portfolio_.position(primary_instrument_).quantity;
  result.realized_pnl_cents = result.final_cash_cents - initial_cash_;
  return result;
}

}  // namespace qflow
