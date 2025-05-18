#pragma once

#include <string>
#include <vector>

namespace Exchange
{
  using OrderId = std::string;
  using Price = int;
  using Quantity = unsigned int;

  enum class OrderType
  {
      IOC, // Immediate or Cancel
      GFD, // Good For Day
  };

  enum class Side
  {
      Buy,  // Bid
      Sell, // Ask
  };

  class Order final
  {
      public:
          Order(Side s, OrderType t, Price p, Quantity q, const OrderId& i)
              : side(s), type(t), price(p), quantity(q), id(i)
          {}
      public:
          Side side;
          OrderType type;
          Price price;
          Quantity quantity;
          OrderId id;
  };

  class Trade final
  {
      public:
          Trade(Order& o1, Order& o2, Quantity q)
              : oppOrder(o1), order(o2), quantity(q) 
              {}
          
          Order oppOrder;
          Order order;
          Quantity quantity;
  };
  using Trades = std::vector<Trade>;

}