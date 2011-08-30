#ifndef IBAPI_COMMON_H_
#define IBAPI_COMMON_H_

#include "ib/tick_types.hpp"
#include "utils.hpp"


namespace IBAPI {

typedef int64 TimestampMicros;

enum SecurityType
{
  STK = 1,
  OPT
};

enum Exchange
{
  SMART = 1
};

enum Currency
{
  USD = 1
};



} // IBAPI


#endif // IBAPI_COMMON_H_

