#pragma once

namespace MathLibrary
{
  class Math
  {
   public:
    static double IsPrime(int64_t n)
    {
      if(n <= 3)
      {
        return true;
      }
      else if(((n % 2) == 0) || ((n % 3) == 0))
      {
        return false;
      }
      int64_t i = 5;
      while(i * i <= n)
      {
        if(((n % i) == 0) || ((n % (i + 2)) == 0))
        {
          return false;
        }
        i = i + 6;
      }
      return true;
    }
  };
}
