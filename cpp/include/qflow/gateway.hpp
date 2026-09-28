#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "qflow/order.hpp"
#include "qflow/portfolio.hpp"
#include "qflow/risk.hpp"

namespace qflow {

enum class TradingMode : std::uint8_t {
  /** Backtest / sim: risk check then book on local Portfolio. */
  Simulation = 0,
  /**
   * Paper/live boundary: risk check, record an outbound intent, and optionally
   * book locally. Does not open a network socket (venue I/O is a later topic).
   */
  PaperLive = 1,
};

/** Outbound order intent captured by PaperLive gateway (audit trail). */
struct OutboundOrder {
  Order order{};
  TradingMode mode{TradingMode::PaperLive};
};

/**
 * Order entry point used by strategies. Always runs RiskGate before side effects.
 */
class OrderGateway {
 public:
  virtual ~OrderGateway() = default;
  virtual TradingMode mode() const noexcept = 0;
  virtual OrderId submit(Order order, std::string* error_message = nullptr) = 0;
};

class SimOrderGateway : public OrderGateway {
 public:
  SimOrderGateway(Portfolio& portfolio, RiskGate& risk, double tick_size);

  TradingMode mode() const noexcept override { return TradingMode::Simulation; }
  OrderId submit(Order order, std::string* error_message = nullptr) override;

 private:
  Portfolio& portfolio_;
  RiskGate& risk_;
  double tick_size_;
};

/**
 * Simulates the live boundary: risk → append outbound log → book on portfolio
 * (paper). Swap the booking step for a real venue client later without changing
 * strategy code.
 */
class PaperLiveGateway : public OrderGateway {
 public:
  PaperLiveGateway(Portfolio& portfolio, RiskGate& risk, double tick_size,
                   bool book_locally = true);

  TradingMode mode() const noexcept override { return TradingMode::PaperLive; }
  OrderId submit(Order order, std::string* error_message = nullptr) override;

  const std::vector<OutboundOrder>& outbound() const noexcept { return outbound_; }

 private:
  Portfolio& portfolio_;
  RiskGate& risk_;
  double tick_size_;
  bool book_locally_;
  std::vector<OutboundOrder> outbound_{};
  OrderId next_outbound_id_{1};
};

}  // namespace qflow
