#pragma once

#include "Order.hpp"

namespace Exchange
{
    class TradeReporter {

        public:
            TradeReporter() = default;
            virtual ~TradeReporter() = default;

            virtual void OnTradeReporter(Trade&& trade) = 0;
    
        private:
            TradeReporter(const TradeReporter &) = delete;
            TradeReporter &operator=(const TradeReporter &) = delete;
    };
}
