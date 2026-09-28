#pragma once

#include <string>
#include <variant>

#include "qflow/engine.hpp"
#include "qflow/gateway.hpp"
#include "qflow/order.hpp"
#include "qflow/portfolio.hpp"

namespace qflow {

/** Services a strategy may use while handling an event (no I/O). */
class StrategyContext {
 public:
  StrategyContext(Portfolio& portfolio, double tick_size, const SimClock& clock,
                  OrderGateway* gateway = nullptr)
      : portfolio_(portfolio),
        tick_size_(tick_size),
        clock_(clock),
        gateway_(gateway) {}

  Portfolio& portfolio() noexcept { return portfolio_; }
  const Portfolio& portfolio() const noexcept { return portfolio_; }
  double tick_size() const noexcept { return tick_size_; }
  const SimClock& clock() const noexcept { return clock_; }
  OrderGateway* gateway() const noexcept { return gateway_; }

  OrderId place_limit(InstrumentId instrument_id, Side side, PriceTicks limit_price,
                      Quantity quantity, std::string* error = nullptr) {
    Order order{};
    order.instrument_id = instrument_id;
    order.side = side;
    order.type = OrderType::Limit;
    order.limit_price = limit_price;
    order.quantity = quantity;
    order.created_ts_ns = clock_.now();
    return submit(order, error);
  }

  /**
   * Market order. price_hint is used by RiskGate notional checks (and ignored by
   * the matcher, which fills at trade price).
   */
  OrderId place_market(InstrumentId instrument_id, Side side, Quantity quantity,
                       std::string* error = nullptr, PriceTicks price_hint = 0) {
    Order order{};
    order.instrument_id = instrument_id;
    order.side = side;
    order.type = OrderType::Market;
    order.limit_price = price_hint;
    order.quantity = quantity;
    order.created_ts_ns = clock_.now();
    return submit(order, error);
  }

 private:
  OrderId submit(Order order, std::string* error) {
    if (gateway_ != nullptr) {
      return gateway_->submit(order, error);
    }
    return portfolio_.place(order, error);
  }

  Portfolio& portfolio_;
  double tick_size_;
  const SimClock& clock_;
  OrderGateway* gateway_{nullptr};
};

/**
 * Strategy receives typed market callbacks. It decides; it does not match fills.
 * Default methods are no-ops so subclasses override only what they need.
 */
class Strategy : public EventHandler {
 public:
  void on_event(const Event& event, const SimClock& clock) override;

  virtual void on_tick(const Tick& tick, StrategyContext& ctx) {
    (void)tick;
    (void)ctx;
  }
  virtual void on_bar(const Bar& bar, StrategyContext& ctx) {
    (void)bar;
    (void)ctx;
  }
  virtual void on_top_of_book(const TopOfBook& book, StrategyContext& ctx) {
    (void)book;
    (void)ctx;
  }
  virtual void on_timer(const TimerEvent& timer, StrategyContext& ctx) {
    (void)timer;
    (void)ctx;
  }

  void bind(Portfolio* portfolio, double tick_size,
            OrderGateway* gateway = nullptr) noexcept {
    portfolio_ = portfolio;
    tick_size_ = tick_size;
    gateway_ = gateway;
  }

 protected:
  Portfolio* portfolio_{nullptr};
  double tick_size_{0.01};
  OrderGateway* gateway_{nullptr};
};

/**
 * Demo strategy: buy a fixed size on the first tick, flat thereafter.
 * Useful for wiring tests before real signal logic (L11).
 */
class BuyOnceStrategy : public Strategy {
 public:
  BuyOnceStrategy(InstrumentId instrument_id, Quantity quantity)
      : instrument_id_(instrument_id), quantity_(quantity) {}

  void on_tick(const Tick& tick, StrategyContext& ctx) override;

 private:
  InstrumentId instrument_id_{0};
  Quantity quantity_{0};
  bool done_{false};
};

}  // namespace qflow
