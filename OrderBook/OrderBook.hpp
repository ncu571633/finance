#pragma once

#include "TradeReporter.hpp"

#include <list>
#include <map>
#include <unordered_map>

namespace Exchange
{
    class OrderBook final
    {
    public:
        OrderBook(TradeReporter& r) : reporter(r) {}
        void AddOrder(Side side, OrderType type, Price price, Quantity quantity, const OrderId& id);
        void CancelOrder(const OrderId& id);
        void ModifyOrder(const OrderId& orderId, Side s, Price price, Quantity quantity);
        void PrintBook();

    private:
        std::map<Price, std::list<Order>> sellBook; // ascending order
        std::map<Price, std::list<Order>, std::greater<Price>> buyBook; // descending order
        std::unordered_map<OrderId, std::pair<Side, std::list<Order>::iterator>> idMap; // id -> ()
        TradeReporter& reporter;
    };
}