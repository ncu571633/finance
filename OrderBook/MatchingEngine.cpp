#include "MatchingEngine.hpp"
#include "OrderBook.hpp"
#include <sstream>

namespace Exchange
{
    void MatchingEngine::Match(std::istream& in, TradeReporter& /*reporter*/)
    {
        std::string line;
        OrderBook orderBook;
        while(getline(in, line))
        {
            std::istringstream iss(line);
            std::string cmd;
            iss >> cmd;
            if (cmd == "BUY" || cmd == "SELL")
            {
                std::string type, orderId;
                int price, quantity;
                iss >> type >> price >> quantity >> orderId;
                if (price<=0 || quantity <=0 || orderId.empty())
                    continue;
                orderBook.AddOrder(ParseSide(cmd), ParseType(type), price, quantity, orderId);
            }
            else if (cmd == "CANCEL")
            {
                std::string orderId;
                iss >> orderId;
                if (orderId.empty())
                    continue;
                orderBook.CancelOrder(orderId);
            }
            else if (cmd == "MODIFY")
            {
                std::string orderId, side;
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
        }
        #if false

        for (std::string line; std::getline(std::cin, line);) 
        {
            auto view = line 
                | std::views::drop_while([](unsigned char c){ return std::isspace(c); })
                | std::views::reverse
                | std::views::drop_while([](unsigned char c){ return std::isspace(c); })
                | std::views::reverse;
            line = std::string(view.begin(), view.end());
            if (line.empty() || line.starts_with('#')) 
            continue;
            std::print("{}\n", line);

            std::istringstream str(line);
            std::string command;
            OrderBook::OrderId id;
            str >> command >> id;
            if (command == "ADD") {
            std::string side_str;
            OrderBook::Price price;
            OrderBook::Quantity quantity;
            str >> side_str >> price >> quantity;
            exchange.on_add(id, OrderBook::parse_side(side_str), price, quantity);
            } else if (command == "DELETE") {
            exchange.on_delete(id);
            } else {
            throw std::runtime_error("Bad command " + command + " in \'" + line + "\'");
            }
        }
  #endif
    }

    Side MatchingEngine::ParseSide(const std::string& side)
    {
        if (side == "BUY") return Side::Buy;
        if (side == "SELL") return Side::Sell;
        throw std::runtime_error("Bad side '" + std::string(side) + "'");
    }

    OrderType MatchingEngine::ParseType(const std::string& s)
    {
        return s == "GFD" ? OrderType::GFD: OrderType::IOC;
    }
}


