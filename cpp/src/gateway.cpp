#include "qflow/gateway.hpp"

namespace qflow {

SimOrderGateway::SimOrderGateway(Portfolio& portfolio, RiskGate& risk,
                                 double tick_size)
    : portfolio_(portfolio), risk_(risk), tick_size_(tick_size) {}

OrderId SimOrderGateway::submit(Order order, std::string* error_message) {
  const RiskDecision decision = risk_.evaluate(portfolio_, order, tick_size_);
  if (!decision.allowed) {
    if (error_message != nullptr) {
      *error_message = decision.reason;
    }
    return 0;
  }
  const OrderId id = portfolio_.place(order, error_message);
  if (id != 0) {
    risk_.on_accept();
  }
  return id;
}

PaperLiveGateway::PaperLiveGateway(Portfolio& portfolio, RiskGate& risk,
                                   double tick_size, bool book_locally)
    : portfolio_(portfolio),
      risk_(risk),
      tick_size_(tick_size),
      book_locally_(book_locally) {}

OrderId PaperLiveGateway::submit(Order order, std::string* error_message) {
  const RiskDecision decision = risk_.evaluate(portfolio_, order, tick_size_);
  if (!decision.allowed) {
    if (error_message != nullptr) {
      *error_message = decision.reason;
    }
    return 0;
  }

  if (!book_locally_) {
    order.id = next_outbound_id_++;
    outbound_.push_back(OutboundOrder{order, TradingMode::PaperLive});
    risk_.on_accept();
    return order.id;
  }

  const OrderId id = portfolio_.place(order, error_message);
  if (id == 0) {
    return 0;
  }
  order.id = id;
  outbound_.push_back(OutboundOrder{order, TradingMode::PaperLive});
  risk_.on_accept();
  return id;
}

}  // namespace qflow
