#include <iomanip>
#include <iostream>
#include <string>

#include "qflow/backtest.hpp"
#include "qflow/feed.hpp"
#include "qflow/gateway.hpp"
#include "qflow/layout.hpp"
#include "qflow/protocol.hpp"
#include "qflow/risk.hpp"
#include "qflow/version.hpp"

int main(int argc, char** argv) {
  std::cout << "QFlow " << qflow::version() << '\n';
  std::cout << "sizeof(Tick)=" << sizeof(qflow::Tick)
            << " cache_line=" << qflow::kCacheLineBytes << '\n';

  const std::string csv_path =
      argc > 1 ? argv[1] : std::string("data/demo_ticks.csv");
  constexpr double kTickSize = 0.01;

  qflow::CsvTickFeed feed;
  std::string err;
  if (!feed.load(csv_path, kTickSize, &err)) {
    std::cerr << "failed to load feed: " << err << '\n';
    return 1;
  }

  qflow::MatchCosts costs;
  costs.slippage_ticks = 1;          // 1 tick adverse
  costs.commission_cents = 100;      // $1.00 per fill

  qflow::Portfolio portfolio(10'000'000);  // $100,000.00
  qflow::DualEmaCrossStrategy strategy(/*instrument*/ 1, /*qty*/ 50,
                                       /*fast*/ 5, /*slow*/ 20);
  qflow::Backtester backtester(portfolio, strategy, kTickSize, 1, costs);
  const qflow::BacktestResult result = backtester.run(feed);

  std::cout << std::fixed << std::setprecision(4);
  std::cout << "backtest csv=" << csv_path
            << " events=" << result.events_delivered
            << " fills=" << result.fills
            << " closed_trades=" << result.closed_trades << '\n'
            << "  position=" << result.final_position
            << " cash_cents=" << result.final_cash_cents
            << " equity_cents=" << result.final_equity_cents << '\n'
            << "  realized_pnl_cents=" << result.realized_pnl_cents
            << " total_return=" << result.total_return
            << " max_drawdown=" << result.max_drawdown
            << " win_rate=" << result.win_rate << '\n';

  // Risk + paper/live boundary demo (no socket).
  qflow::RiskLimits limits;
  limits.max_order_qty = 50;
  qflow::RiskGate gate(limits);
  qflow::Portfolio paper(1'000'000);
  qflow::PaperLiveGateway live(paper, gate, kTickSize, true);
  qflow::Order big{};
  big.instrument_id = 1;
  big.side = qflow::Side::Buy;
  big.type = qflow::OrderType::Limit;
  big.limit_price = 19000;
  big.quantity = 100;
  const qflow::OrderId rejected = live.submit(big, &err);
  std::cout << "risk reject id=" << rejected << " err=" << err
            << " mode=" << static_cast<int>(live.mode()) << '\n';

  const qflow::Tick sample{1, 19025, 10, 1, qflow::Side::Buy};
  std::cout << "wire=" << qflow::format_tick_line(sample) << '\n';
  return 0;
}
