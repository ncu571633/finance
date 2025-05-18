#pragma once

#include "TradeReporter.hpp"
#include "Types.hpp"

#include <list>
#include <map>
#include <unordered_map>
#include <vector>

namespace Exchange
{
    class Trade final
    {
        public:
            Trade(OrderId askOrderId, OrderId bidOrderId, Price price, Quantity quantity)
                : askOrderId_(askOrderId), bidOrderId_(bidOrderId), price_(price), quantity_(quantity) 
                {}
        private:
            OrderId askOrderId_;
            OrderId bidOrderId_;
            Price price_;
            Quantity quantity_;
    };
    using Trades = std::vector<Trade>;

    class Order final
    {
        public:
            Order(Side s, OrderType t, int p, int q, std::string i)
                : side(s), type(t), price(p), quantity(q), id(i)
            {
            }
        public:
            Side side;
            OrderType type;
            Price price;
            Quantity quantity;
            OrderId id;
    };

    class OrderBook
    {
    public:
        void AddOrder(Side side, OrderType type, int price, int quantity, const std::string& id);
        void CancelOrder(const std::string& id);
        void ModifyOrder(std::string orderId, Side s, int price, int quantity);
        void PrintBook();

        private:
        std::map<int, std::list<Order>> sellBook; // ascending order
        std::map<int, std::list<Order>, std::greater<int>> buyBook; // descending order
        std::unordered_map<std::string, std::pair<Side, std::list<Order>::iterator>> idMap; // id -> ()
        
    };
}