#include "OrderBook.hpp" 
#include <print>
#include <iostream>

namespace Exchange
{
    void OrderBook::AddOrder(Side side, OrderType type, int price, int quantity, const std::string& id)
    {
        Order order(side, type, price, quantity, id);

        if (side == Side::Buy)
        {
            // check sellBook, which increases          
            // start from the beginning
            for (std::map<int, std::list<Order>>::iterator iter = sellBook.begin();
                iter!=sellBook.end() && iter->first <= order.price && order.quantity > 0;
            )
            {
                std::list<Order>& sellOrderList = iter->second;
                for(std::list<Order>::iterator sellOrderIter = sellOrderList.begin();
                    sellOrderIter != sellOrderList.end() && order.quantity > 0;
                )
                {
                    Order& sellOrder = *sellOrderIter;
                    int tradeQuantity = std::min(sellOrder.quantity, order.quantity);
                    std::cout << "TRADE " << sellOrder.id << " " << sellOrder.price << " " << tradeQuantity
                              << " " << order.id << " " << order.price << " " << tradeQuantity
                              << std::endl;
                    order.quantity -= tradeQuantity;
                    sellOrder.quantity -= tradeQuantity;

                    if (sellOrder.quantity == 0)
                    {
                        sellOrderIter = sellOrderList.erase(sellOrderIter);
                        idMap.erase(sellOrder.id);
                    }
                    else { // sellOrder.quantity > 0 and order.quantity == 0
                        break;
                    }
                }

                if (sellOrderList.empty())
                    iter = sellBook.erase(iter);
                else
                    break;
                if (order.quantity == 0)
                    break;
            }

            // ignore IOC: if not fully matched, skip
            if (order.quantity > 0 && order.type == OrderType::GFD)
            {
                auto& book = buyBook[order.price];
                book.push_back(order);
                idMap[order.id] = {order.side, --book.end()};
            }
        }
        else // order.side == Side::Sell
        {
            // check buyBook, which decreases
            for(auto iter = buyBook.begin();
                iter!=buyBook.end() && order.quantity > 0 && iter->first >= order.price;
            )
            {
                std::list<Order>& buyOrderList = iter->second;
                for (auto buyOrderIter = buyOrderList.begin();
                    buyOrderIter != buyOrderList.end() && order.quantity > 0;
                )
                {
                    Order& buyOrder = *buyOrderIter;
                    int tradeQuantity = std::min(order.quantity, buyOrder.quantity);
                    std::cout << "TRADE " << buyOrder.id << " " << buyOrder.price << " " << tradeQuantity
                              << " " << order.id << " " << order.price << " " << tradeQuantity
                              << std::endl;
                    order.quantity -= tradeQuantity;
                    buyOrder.quantity -= tradeQuantity;
                    if (buyOrder.quantity == 0)
                    {
                        buyOrderIter = buyOrderList.erase(buyOrderIter);
                        idMap.erase(buyOrder.id);
                    }
                    else { // buyOrder.quantity > 0 and order.quantity == 0
                        break;
                    }
                }

                if (buyOrderList.empty())
                    iter = buyBook.erase(iter);
                else
                    break;
                if (order.quantity == 0)
                    break;
            }

            // ignore IOC: if not fully matched, skip
            if (order.quantity > 0 && order.type == OrderType::GFD)
            {
                auto& book = sellBook[order.price];
                book.push_back(order);
                idMap[order.id] = {order.side, --book.end()};
            }
        }
    }

    void OrderBook::CancelOrder(const std::string& id)
    {
        auto it = idMap.find(id);
        // cannot find Order
        if (it == idMap.end())
            return ;

        Side side = it->second.first;
        auto& orderIter = it->second.second;
        int price = orderIter->price;
        if (side == Side::Buy)
        {
            buyBook[price].erase(orderIter);
            if (buyBook[price].empty())
            {
                buyBook.erase(price);
            }
        }
        else {
            sellBook[price].erase(orderIter);
            if (sellBook[price].empty())
            {
                sellBook.erase(price);
            }
        }
        idMap.erase(it);
    }

    void OrderBook::ModifyOrder(std::string orderId, Side s, int price, int quantity)
    {
        auto it = idMap.find(orderId);
        // cannot find Order
        if (it == idMap.end())
            return ;

        auto& orderIter = it->second.second;
        Order order = *orderIter;
        if (order.type == OrderType::IOC)
            return ;

        CancelOrder(orderId);

        // insert as a new order, loses priority
        AddOrder(s, OrderType::GFD, price, quantity, orderId);
    }

    void OrderBook::PrintBook()
    {
        std::cout << "SELL:" << std::endl;
        for(auto iter = sellBook.rbegin(); iter!=sellBook.rend(); ++iter) // decerasing order
        {
            int quantity = 0;
            for (auto& orderIter: iter->second)
                quantity += orderIter.quantity;
            std::cout << iter->first << " " << quantity << std::endl;
        }

        std::cout << "BUY:" << std::endl;
        for(auto iter = buyBook.begin(); iter!=buyBook.end(); ++iter) // decreasing order
        {
            int quantity = 0;
            for (auto& orderIter: iter->second)
                quantity += orderIter.quantity;
            std::cout << iter->first << " " << quantity << std::endl;
        }
    }

}
