#ifndef IBAPI_V964_API_MESSAGES_H_
#define IBAPI_V964_API_MESSAGES_H_

#include <cstdio>
#include <boost/format.hpp>
#include <glog/logging.h>

#include <quickfix/FieldConvertors.h>
#include <quickfix/FieldNumbers.h>
#include <ql/quantlib.hpp>

#include <Shared/Contract.h>
#include "ib/internal.hpp"
#include "ib/IBAPIValues.hpp"
#include "ib/ApiMessageBase.hpp"
#include "ib/TickerMap.hpp"


using ib::internal::EClientPtr;

namespace ib {
namespace internal {

using ib::internal::ApiMessageBase;

class V964Message : public ApiMessageBase
{
 public:
  V964Message(const IBAPI::MsgType& msgType) :
      ApiMessageBase(msgType, msgType + ".v964.ib")
  {}

  V964Message(const IBAPI::Message& copy) : ApiMessageBase(copy)
  {}

  ~V964Message() {}
};

} // internal
} // ib




namespace IBAPI {
namespace V964 {

using ib::internal::V964Message;


static std::string FormatExpiry(int year, int month, int day)
{
  ostringstream s1;
  string fmt = (month > 9) ? "%4d%2d" : "%4d0%1d";
  string fmt2 = (day > 9) ? "%2d" : "0%1d";
  s1 << boost::format(fmt) % year % month << boost::format(fmt2) % day;
  return s1.str();
}

static void FormatExpiry(V964Message& message,
                         int year, int month, int day)
{
  std::ostringstream yearMonth;
  yearMonth << boost::format((month > 9) ? "%4d%2d" : "%4d0%1d") % year % month;
  message.setField(FIX::MaturityMonthYear(yearMonth.str()));

  std::ostringstream dayOfMonth;
  dayOfMonth << boost::format((day > 9) ? "%2d" : "0%1d") % day;
  message.setField(FIX::MaturityDay(dayOfMonth.str()));
}

static void FormatExpiry(V964Message& message, QuantLib::Date date)
{
  FormatExpiry(message, date.year(), date.month(), date.dayOfMonth());
}


/** Example twsContract (see R/IBrokers module):
 $ conId          : chr "96099040"
 $ symbol         : chr "GOOG"
 $ sectype        : chr "OPT"
 $ exch           : chr "SMART"
 $ primary        : chr ""
 $ expiry         : chr "20111028"
 $ strike         : chr "590"
 $ currency       : chr "USD"
 $ right          : chr "C"
 $ local          : chr "GOOG  111028C00590000"
 $ multiplier     : chr "100"
 $ combo_legs_desc: chr ""
 $ comboleg       : chr ""
 $ include_expired: chr ""
 $ secIdType      : chr ""
 $ secId          : chr ""
*/
class MarketDataRequest : public V964Message
{
 public:

  MarketDataRequest() : V964Message("MarketDataRequest")
  {
  }

  MarketDataRequest(const IBAPI::Message& copy) : V964Message(copy)
  {
  }

  ~MarketDataRequest()
  {
  }

  // The following maps as subset of FIX41 MarketDataRequest::NoRelatedSym group
  FIELD_SET(*this, FIX::Symbol);
  FIELD_SET(*this, FIX::MDEntryRefID); // --> conId
  FIELD_SET(*this, FIX::SecurityID);
  FIELD_SET(*this, FIX::SecurityType);
  FIELD_SET(*this, FIX::DerivativeSecurityID); // --> local
  FIELD_SET(*this, FIX::SecurityExchange);
  FIELD_SET(*this, FIX::PutOrCall);
  FIELD_SET(*this, FIX::StrikePrice);
  FIELD_SET(*this, FIX::Currency);
  FIELD_SET(*this, FIX::ContractMultiplier);
  FIELD_SET(*this, FIX::MaturityMonthYear);
  FIELD_SET(*this, FIX::MaturityDay);


  void marshall(Contract& contract)
  {
    MAP_REQUIRED_FIELD(FIX::Symbol, contract.symbol);

    REQUIRED_FIELD(FIX::SecurityType, securityType);
    if (securityType == FIX::SecurityType_OPTION) {

      MAP_REQUIRED_FIELD(FIX::StrikePrice, contract.strike);

      REQUIRED_FIELD(FIX::MaturityMonthYear, expiryMonthYear);
      REQUIRED_FIELD(FIX::MaturityDay, expiryDay);
      contract.expiry = expiryMonthYear.getString() + expiryDay.getString();

      REQUIRED_FIELD(FIX::PutOrCall, putOrCall);
      contract.secType = "OPT";
      switch (putOrCall) {
        case FIX::PutOrCall_PUT:
          contract.right = "P";
          break;
        case FIX::PutOrCall_CALL:
          contract.right = "C";
          break;
      }

      MAP_OPTIONAL_FIELD(FIX::DerivativeSecurityID, contract.localSymbol);
      OPTIONAL_FIELD_DEFAULT(FIX::ContractMultiplier, multiplier, 100);
      // Type conversion from FIX INT to string
      contract.multiplier = multiplier.getString();

    } else if (securityType == FIX::SecurityType_COMMON_STOCK) {
      contract.secType= "STK";
    }

    MAP_OPTIONAL_FIELD(FIX::SecurityID, contract.secId);
    MAP_OPTIONAL_FIELD_DEFAULT(FIX::SecurityExchange, contract.exchange, SMART);
    MAP_OPTIONAL_FIELD_DEFAULT(FIX::Currency, contract.currency, USD);

    //contract.conId, also used for tickerId
    OPTIONAL_FIELD(FIX::MDEntryRefID, conId);
    if (conId.getLength() > 0) {
      long conIdLong = 0L;
      if (FIX::IntConvertor::convert(conId.getValue(), conIdLong)) {
        contract.conId = conIdLong;
      }
    }
  }

  bool callApi(EClientPtr eclient)
  {
    Contract contract;
    try {
      marshall(contract);
    } catch (FIX::FieldNotFound e) {
      return false;
    }

    ib::internal::TickerMap tm;
    long tickerId = tm.registerContract(contract);

    if (tickerId > 0) {
      eclient->reqMktData(tickerId, contract, "", false);
      return true;
    }
    return false;
  }
};


class CancelMarketDataRequest : public V964Message
{
 public:

  CancelMarketDataRequest() : V964Message("CancelMarketDataRequest")
  {
  }

  CancelMarketDataRequest(const IBAPI::Message& copy) : V964Message(copy)
  {
  }

  ~CancelMarketDataRequest()
  {
  }

  // The following maps as subset of FIX41 MarketDataRequest::NoRelatedSym group
  FIELD_SET(*this, FIX::Symbol);
  FIELD_SET(*this, FIX::MDEntryRefID); // --> conId
  FIELD_SET(*this, FIX::DerivativeSecurityID); // --> local


  long marshall()
  {
    ib::internal::TickerMap tm;

    std::string symbol("");
    std::string localSymbol("");

    MAP_OPTIONAL_FIELD(FIX::Symbol, symbol);
    MAP_OPTIONAL_FIELD(FIX::DerivativeSecurityID, localSymbol);

    long tickerId = -1;
    OPTIONAL_FIELD(FIX::MDEntryRefID, conId);
    if (conId.getLength() > 0) {
      long conIdLong = 0L;
      if (FIX::IntConvertor::convert(conId.getValue(), conIdLong)) {
        tickerId = conIdLong;
      }
    } else if (localSymbol != "") {
      tm.getTickerIdFromSubscriptionKey(localSymbol, &tickerId);
    } else if (symbol != "") {
      tm.getTickerIdFromSubscriptionKey(symbol, &tickerId);
    }
    return tickerId;
  }

  bool callApi(EClientPtr eclient)
  {
    long tickerId = 0;
    try {
      tickerId = marshall();
    } catch (FIX::FieldNotFound e) {
      return false;
    }
    if (tickerId > 0) {
      eclient->cancelMktData(tickerId);
      return true;
    }
    return false;
  }
};


class OptionChainRequest : public V964Message
{
 public:

  OptionChainRequest() : V964Message("OptionChainRequest")
  {
  }

  OptionChainRequest(const IBAPI::Message& copy) : V964Message(copy)
  {
  }

  ~OptionChainRequest()
  {
  }

  FIELD_SET(*this, FIX::Symbol);
  FIELD_SET(*this, FIX::SecurityExchange);

  // If specified, filter by side
  FIELD_SET(*this, FIX::PutOrCall);
  FIELD_SET(*this, FIX::Currency);

  // If specified, filter all contracts by expiry
  FIELD_SET(*this, FIX::MaturityMonthYear);
  FIELD_SET(*this, FIX::MaturityDay);


  void marshall(Contract& contract)
  {
    MAP_REQUIRED_FIELD(FIX::Symbol, contract.symbol);

    contract.secType = "OPT";

    OPTIONAL_FIELD_DEFAULT(FIX::PutOrCall, putOrCall, -1);
    switch (putOrCall) {
      case FIX::PutOrCall_PUT:
        contract.right = "P";
        break;
      case FIX::PutOrCall_CALL:
        contract.right = "C";
        break;
    }

    OPTIONAL_FIELD(FIX::MaturityMonthYear, expiryMonthYear);
    OPTIONAL_FIELD(FIX::MaturityDay, expiryDay);

    if (expiryMonthYear.getLength() > 0 && expiryDay.getLength() > 0) {
      contract.expiry = expiryMonthYear.getString() + expiryDay.getString();
    }

    MAP_OPTIONAL_FIELD_DEFAULT(FIX::SecurityExchange, contract.exchange, SMART);
    MAP_OPTIONAL_FIELD_DEFAULT(FIX::Currency, contract.currency, USD);
  }

  bool callApi(EClientPtr eclient)
  {
    Contract contract;
    try {
      marshall(contract);
    } catch (FIX::FieldNotFound e) {
      return false;
    }

    return true;
  }
};


} // V964
} // IBAPI



#endif // IBAPI_V964_API_MESSAGES_H_
