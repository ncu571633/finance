#include "MatchingEngine.hpp"
#include "OrderBook.hpp"
#include <sstream>
#include <ranges>

namespace Exchange
{
    void MatchingEngine::Match(std::istream& in, TradeReporter& reporter)
    {
        OrderBook orderBook(reporter);

        for (std::string line; 
            std::getline(in, line);
        )
        {
            auto view = line 
                | std::views::drop_while([](unsigned char c){ return std::isspace(c); })
                | std::views::reverse
                | std::views::drop_while([](unsigned char c){ return std::isspace(c); })
                | std::views::reverse;
            line = std::string(view.begin(), view.end());
            if (line.empty() || line.starts_with('#')) 
                continue;
            // std::print("{}\n", line);

            std::istringstream iss(line);
            std::string cmd;
            iss >> cmd;
            if (cmd == "BUY" || cmd == "SELL")
            {
                std::string type;
                OrderId orderId;
                int price, quantity;
                iss >> type >> price >> quantity >> orderId;
                if (price<=0 || quantity <=0 || orderId.empty())
                    continue;
                orderBook.AddOrder(ParseSide(cmd), ParseType(type), price, quantity, orderId);
            }
            else if (cmd == "CANCEL")
            {
                OrderId orderId;
                iss >> orderId;
                if (orderId.empty())
                    continue;
                orderBook.CancelOrder(orderId);
            }
            else if (cmd == "MODIFY")
            {
                std::string side;
                OrderId orderId;
                int price, quantity;
                iss >> orderId >> side >> price >> quantity;
                if (price<=0 || quantity <=0 || orderId.empty())
                    continue;
                orderBook.ModifyOrder(orderId, ParseSide(side), price, quantity);
            }
            else if (cmd == "PRINT")
            {
                orderBook.PrintBook();
            }
            else 
            {
                throw std::runtime_error("Bad command " + cmd + " in \'" + line + "\'");
            }
        }
    }

    Side MatchingEngine::ParseSide(const std::string& side)
    {
        if (side == "BUY") return Side::Buy;
        if (side == "SELL") return Side::Sell;
        throw std::runtime_error("Bad side '" + std::string(side) + "'");
    }

    OrderType MatchingEngine::ParseType(const std::string& type)
    {
        if (type == "GFD") return OrderType::GFD;
        if (type == "IOC") return OrderType::IOC;
        throw std::runtime_error("Bad type'" + type + "'");
    }
}


