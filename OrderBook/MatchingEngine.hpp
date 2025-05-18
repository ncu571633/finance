#pragma once

#include <iostream>
#include "TradeReporter.hpp"

namespace Exchange
{
    class MatchingEngine
    {
    public:
        static void Match(std::istream& in, TradeReporter& reporter);

    private:
        static Side ParseSide(const std::string& side);
        static OrderType ParseType(const std::string& s);
    };
}