#pragma once

#include <stdexcept>
#include <string>
#include <string_view>

namespace Exchange
{

  using OrderId = std::string;
  using Price = int;
  using Quantity = unsigned int;

  enum class OrderType
  {
      IOC,
      GFD,
  };

  enum class Side
  {
      Buy,  // Bid
      Sell, // Ask
  };

}