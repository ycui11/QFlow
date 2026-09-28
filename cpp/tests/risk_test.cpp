#include <cstdlib>
#include <iostream>
#include <string>

#include "qflow/gateway.hpp"
#include "qflow/risk.hpp"
#include "qflow/strategy.hpp"

namespace {

int failures = 0;
void expect(bool c, const char* m) {
  if (!c) {
    std::cerr << "FAIL: " << m << '\n';
    ++failures;
  }
}

}  // namespace

int main() {
  constexpr double kTick = 0.01;

  {
    qflow::RiskLimits lim;
    lim.max_order_qty = 100;
    qflow::RiskGate gate(lim);
    qflow::Portfolio book(5'000'000);
    qflow::Order order{};
    order.instrument_id = 1;
    order.side = qflow::Side::Buy;
    order.type = qflow::OrderType::Limit;
    order.limit_price = 19000;
    order.quantity = 101;
    auto d = gate.evaluate(book, order, kTick);
    expect(!d.allowed, "reject oversized order");
  }

  {
    qflow::RiskLimits lim;
    lim.max_abs_position = 50;
    qflow::RiskGate gate(lim);
    qflow::Portfolio book(5'000'000);
    qflow::Order order{};
    order.instrument_id = 1;
    order.side = qflow::Side::Buy;
    order.type = qflow::OrderType::Limit;
    order.limit_price = 10000;
    order.quantity = 40;
    expect(gate.evaluate(book, order, kTick).allowed, "40 ok");
    book.place(order);
    qflow::Fill fill{1, 1, 1, qflow::Side::Buy, 10000, 40};
    book.apply_fill(fill, kTick);
    order.quantity = 20;
    expect(!gate.evaluate(book, order, kTick).allowed, "would exceed position");
  }

  {
    qflow::RiskLimits lim;
    lim.max_orders_per_session = 1;
    qflow::RiskGate gate(lim);
    qflow::Portfolio book(5'000'000);
    qflow::SimOrderGateway gw(book, gate, kTick);
    qflow::Order order{};
    order.instrument_id = 1;
    order.side = qflow::Side::Buy;
    order.type = qflow::OrderType::Limit;
    order.limit_price = 10000;
    order.quantity = 1;
    expect(gw.submit(order) != 0, "first accepted");
    expect(gw.submit(order) == 0, "second blocked by session cap");
  }

  {
    qflow::RiskGate gate({});
    gate.kill();
    qflow::Portfolio book(5'000'000);
    qflow::SimOrderGateway gw(book, gate, kTick);
    qflow::Order order{};
    order.instrument_id = 1;
    order.side = qflow::Side::Buy;
    order.type = qflow::OrderType::Market;
    order.quantity = 1;
    order.limit_price = 10000;
    std::string err;
    expect(gw.submit(order, &err) == 0, "kill switch");
    expect(err.find("kill") != std::string::npos ||
               err.find("disabled") != std::string::npos,
           "kill reason");
  }

  {
    qflow::RiskLimits lim;
    lim.max_order_qty = 1000;
    qflow::RiskGate gate(lim);
    qflow::Portfolio book(5'000'000);
    qflow::PaperLiveGateway live(book, gate, kTick, true);
    qflow::BuyOnceStrategy strat(1, 10);
    strat.bind(&book, kTick, &live);

    qflow::EventEngine engine;
    engine.set_handler(&strat);
    engine.post(qflow::make_tick_event(
        qflow::Tick{1, 19000, 1, 1, qflow::Side::Buy}));
    engine.run();

    expect(live.mode() == qflow::TradingMode::PaperLive, "paper mode");
    expect(live.outbound().size() == 1, "one outbound intent");
    expect(book.open_orders().size() == 1, "booked locally");
  }

  if (failures) {
    std::cerr << failures << " failed\n";
    return EXIT_FAILURE;
  }
  std::cout << "All risk/gateway checks passed\n";
  return EXIT_SUCCESS;
}
