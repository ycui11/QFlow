#include <cstdlib>
#include <iostream>
#include <string>

#include "qflow/portfolio.hpp"

namespace {

int failures = 0;

void expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    ++failures;
  }
}

}  // namespace

int main() {
  constexpr double kTickSize = 0.01;

  expect(qflow::notional_cents(19025, 100, kTickSize) == 1'902'500,
         "notional 190.25 * 100 = 1902500 cents");

  {
    // $30,000.00 — enough for 100 * $190
    qflow::Portfolio book(3'000'000);
    qflow::Order order{};
    order.instrument_id = 1;
    order.side = qflow::Side::Buy;
    order.type = qflow::OrderType::Limit;
    order.limit_price = 19000;
    order.quantity = 100;
    order.created_ts_ns = 1;

    std::string err;
    const qflow::OrderId id = book.place(order, &err);
    expect(id != 0, "place ok");
    expect(book.find_order(id)->status == qflow::OrderStatus::Accepted,
           "accepted");
    expect(book.open_orders().size() == 1, "one open order");

    // 40 * $190.00 = $7,600.00 -> 760000 cents
    qflow::Fill fill{id, 1, 10, qflow::Side::Buy, 19000, 40};
    expect(book.apply_fill(fill, kTickSize, &err), "partial fill");
    expect(book.find_order(id)->status == qflow::OrderStatus::PartiallyFilled,
           "partially filled");
    expect(book.find_order(id)->filled == 40, "filled 40");
    expect(book.position(1).quantity == 40, "long 40");
    expect(book.cash_cents() == 3'000'000 - 760'000, "cash after partial");

    fill.quantity = 60;
    fill.ts_ns = 11;
    expect(book.apply_fill(fill, kTickSize, &err), "final fill");
    expect(book.find_order(id)->status == qflow::OrderStatus::Filled, "filled");
    expect(book.open_orders().empty(), "no open orders");
    expect(book.position(1).quantity == 100, "long 100");
    expect(book.position(1).avg_price == 19000, "avg 190.00");
    // 100 * $190 = $19,000 -> 1_900_000 cents
    expect(book.cash_cents() == 3'000'000 - 1'900'000, "cash after full buy");
  }

  {
    qflow::Portfolio book(100'000);  // $1,000.00
    qflow::Order order{};
    order.instrument_id = 1;
    order.side = qflow::Side::Buy;
    order.type = qflow::OrderType::Limit;
    order.limit_price = 20000;
    order.quantity = 1000;  // needs $20,000
    order.created_ts_ns = 1;
    const qflow::OrderId id = book.place(order);
    qflow::Fill fill{id, 1, 1, qflow::Side::Buy, 20000, 1000};
    std::string err;
    expect(!book.apply_fill(fill, kTickSize, &err), "reject insufficient cash");
    expect(book.cash_cents() == 100'000, "cash unchanged");
    expect(book.position(1).quantity == 0, "flat");
  }

  {
    qflow::Portfolio book(500'000);  // $5,000.00
    qflow::Order buy{};
    buy.instrument_id = 2;
    buy.side = qflow::Side::Buy;
    buy.type = qflow::OrderType::Market;
    buy.quantity = 10;
    buy.created_ts_ns = 1;
    const qflow::OrderId buy_id = book.place(buy);
    expect(book.apply_fill({buy_id, 2, 1, qflow::Side::Buy, 10000, 10}, kTickSize),
           "buy 10 @ 100");

    qflow::Order sell{};
    sell.instrument_id = 2;
    sell.side = qflow::Side::Sell;
    sell.type = qflow::OrderType::Market;
    sell.quantity = 4;
    sell.created_ts_ns = 2;
    const qflow::OrderId sell_id = book.place(sell);
    expect(
        book.apply_fill({sell_id, 2, 2, qflow::Side::Sell, 11000, 4}, kTickSize),
        "sell 4 @ 110");
    expect(book.position(2).quantity == 6, "remaining long 6");
    // -10*$100 + 4*$110 = -1000 + 440 = -560 dollars -> -56000 cents
    expect(book.cash_cents() == 500'000 - 100'000 + 44'000, "cash after round");
  }

  {
    qflow::Portfolio book(100);
    std::string err;
    qflow::Order bad{};
    bad.instrument_id = 1;
    bad.side = qflow::Side::Buy;
    bad.type = qflow::OrderType::Limit;
    bad.quantity = 0;
    expect(book.place(bad, &err) == 0, "reject zero qty");
  }

  if (failures != 0) {
    std::cerr << failures << " test(s) failed\n";
    return EXIT_FAILURE;
  }
  std::cout << "All portfolio checks passed\n";
  return EXIT_SUCCESS;
}
