#include "MatchingEngine.hpp"
#include "TradeReporter.hpp"
#include "OrderBook.hpp"

#include <iostream>
#include <print>
#include <sstream>
#include <stdexcept>
#include <string>
#include <ranges>

namespace Exchange 
{
namespace {
  class PrintingReporter : public TradeReporter {
  public:
    virtual void OnTradeReporter(Trade&& trade) override
    {
      std::print("TRADE {} {} {} {} {} {}\n", 
            trade.oppOrder.id, trade.oppOrder.price, trade.quantity,
            trade.order.id, trade.order.price, trade.quantity
      );
    }
  };
}
}

int main(/*int argc, void **argv*/)
{
    Exchange::PrintingReporter reporter;
    Exchange::MatchingEngine::Match(std::cin, reporter);
    return 0;
}
