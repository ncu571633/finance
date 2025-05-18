#pragma once

#include "Types.hpp"

namespace Exchange
{
    class TradeReporter {

        public:
            TradeReporter() = default;
            virtual ~TradeReporter() = default;

            virtual void OnTradeImpl(OrderId order_id, Price price, Quantity quantity) = 0;
#if false
            template<typename Self>
            void onTrade(this Self&& self, OrderId order_id, Price price, Quantity quantity)
            {
                static_cast<Self*>(this)->onTradeImpl(order_id, price, quantity);
            }
#endif

            TradeReporter(const TradeReporter &) = delete;
            TradeReporter &operator=(const TradeReporter &) = delete;
    };
}
