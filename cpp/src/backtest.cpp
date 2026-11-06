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

PriceTicks apply_slippage(Side side, PriceTicks trade_price,
                          PriceTicks slippage_ticks) noexcept {
  if (slippage_ticks <= 0) {
    return trade_price;
  }
  if (side == Side::Buy) {
    return trade_price + slippage_ticks;
  }
  const PriceTicks slipped = trade_price - slippage_ticks;
  return slipped > 0 ? slipped : 1;
}

MoneyCents equity_cents(const Portfolio& portfolio, InstrumentId instrument_id,
                        PriceTicks last_price, double tick_size) noexcept {
  MoneyCents equity = portfolio.cash_cents();
  const Position pos = portfolio.position(instrument_id);
  if (pos.quantity != 0 && last_price > 0) {
    equity += notional_cents(last_price, pos.quantity, tick_size);
  }
  return equity;
}

/** Tracks long-only round trips for a rough win-rate statistic. */
class RoundTripTracker {
 public:
  void on_fill(const Fill& fill, double tick_size) {
    const Quantity signed_qty =
        fill.side == Side::Buy ? fill.quantity : -fill.quantity;
    const Quantity old_qty = qty_;
    const Quantity new_qty = old_qty + signed_qty;

    if (old_qty > 0 && signed_qty < 0) {
      const Quantity closed =
          std::min(old_qty, static_cast<Quantity>(-signed_qty));
      const MoneyCents entry = notional_cents(avg_price_, closed, tick_size);
      const MoneyCents exit = notional_cents(fill.price, closed, tick_size);
      const MoneyCents pnl = exit - entry;
      ++closed_trades_;
      if (pnl > 0) {
        ++winning_trades_;
      }
    }

    if (old_qty == 0 || (old_qty > 0 && signed_qty > 0)) {
      const Quantity abs_old = old_qty >= 0 ? old_qty : -old_qty;
      const Quantity abs_add = fill.quantity;
      const Quantity denom = abs_old + abs_add;
      if (denom > 0) {
        avg_price_ =
            (avg_price_ * abs_old + fill.price * abs_add) / denom;
      }
    } else if (new_qty == 0) {
      avg_price_ = 0;
    }

    qty_ = new_qty;
  }

  std::size_t closed_trades() const noexcept { return closed_trades_; }
  double win_rate() const noexcept {
    if (closed_trades_ == 0) {
      return 0.0;
    }
    return static_cast<double>(winning_trades_) /
           static_cast<double>(closed_trades_);
  }

 private:
  Quantity qty_{0};
  PriceTicks avg_price_{0};
  std::size_t closed_trades_{0};
  std::size_t winning_trades_{0};
};

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

    const PriceTicks fill_price =
        apply_slippage(order->side, tick.price, costs_.slippage_ticks);
    Fill fill{id, tick.instrument_id, tick.ts_ns, order->side, fill_price, qty};
    std::string err;
    if (!portfolio.apply_fill(fill, tick_size, &err, costs_.commission_cents)) {
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
  Session(Backtester& owner, RoundTripTracker& trades, MoneyCents* peak_equity,
          double* max_drawdown, PriceTicks* last_price)
      : owner_(owner),
        trades_(trades),
        peak_equity_(peak_equity),
        max_drawdown_(max_drawdown),
        last_price_(last_price) {}

  void on_event(const Event& event, const SimClock& clock) override {
    owner_.strategy_.on_event(event, clock);
    if (const auto* tick = std::get_if<Tick>(&event.body)) {
      const std::size_t fills_before = owner_.fills_.size();
      owner_.matcher_.match(owner_.portfolio_, *tick, owner_.tick_size_,
                            &owner_.fills_);
      for (std::size_t i = fills_before; i < owner_.fills_.size(); ++i) {
        trades_.on_fill(owner_.fills_[i], owner_.tick_size_);
      }

      *last_price_ = tick->price;
      const MoneyCents equity =
          equity_cents(owner_.portfolio_, owner_.primary_instrument_,
                       tick->price, owner_.tick_size_);
      if (equity > *peak_equity_) {
        *peak_equity_ = equity;
      }
      if (*peak_equity_ > 0) {
        const double dd = static_cast<double>(*peak_equity_ - equity) /
                          static_cast<double>(*peak_equity_);
        if (dd > *max_drawdown_) {
          *max_drawdown_ = dd;
        }
      }
    }
  }

 private:
  Backtester& owner_;
  RoundTripTracker& trades_;
  MoneyCents* peak_equity_;
  double* max_drawdown_;
  PriceTicks* last_price_;
};

Backtester::Backtester(Portfolio& portfolio, Strategy& strategy, double tick_size,
                       InstrumentId primary_instrument, MatchCosts costs)
    : portfolio_(portfolio),
      strategy_(strategy),
      tick_size_(tick_size),
      primary_instrument_(primary_instrument),
      matcher_(costs),
      initial_cash_(portfolio.cash_cents()) {
  strategy_.bind(&portfolio_, tick_size_);
}

BacktestResult Backtester::run(MarketFeed& feed) {
  fills_.clear();
  initial_cash_ = portfolio_.cash_cents();

  RoundTripTracker trades;
  MoneyCents peak_equity = initial_cash_;
  double max_drawdown = 0.0;
  PriceTicks last_price = 0;

  Session session(*this, trades, &peak_equity, &max_drawdown, &last_price);
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
  result.final_equity_cents =
      equity_cents(portfolio_, primary_instrument_, last_price, tick_size_);
  if (initial_cash_ != 0) {
    result.total_return =
        static_cast<double>(result.final_equity_cents - initial_cash_) /
        static_cast<double>(initial_cash_);
  }
  result.max_drawdown = max_drawdown;
  result.closed_trades = trades.closed_trades();
  result.win_rate = trades.win_rate();
  return result;
}

}  // namespace qflow
