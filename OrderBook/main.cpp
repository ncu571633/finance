#include "MatchingEngine.hpp"
#include "TradeReporter.hpp"

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
  void OnTradeImpl(OrderId order_id, Price price, Quantity quantity)
  {
    std::print("TRADE {} {} {}", order_id, price, quantity);
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
