
#include <ql/quantlib.hpp>

#include "ApiMessages.hpp"

#include "ib/TickerMap.hpp"
#include "ib/VersionSpecific.hpp"

// This is the version-specific header where the implementations
// are to be provided by versioned code at build time.

namespace ib {
namespace testing {


using QuantLib::Date;
using IBAPI::V964::MarketDataRequest;



ib::internal::ZmqSendable*
VersionSpecific::getMarketDataRequestForTest()
{
  MarketDataRequest* request = new MarketDataRequest();

  // Using set(X) as the type-safe way (instead of setField())
  request->set(FIX::Symbol("GOOG"));
  request->set(FIX::SecurityType(FIX::SecurityType_COMMON_STOCK));
  request->set(FIX::SecurityExchange(IBAPI::SecurityExchange_SMART));

  // Date today = Date::todaysDate();
  // Date nextFriday = Date::nextWeekday(today, QuantLib::Friday);
  // IBAPI::V964::FormatExpiry(*request, nextFriday);

  return request;
}

} // namespace testing
} // namespace ib

