#ifndef IBAPI_API_MESSAGES_H_
#define IBAPI_API_MESSAGES_H_

#include <glog/logging.h>

#include <quickfix/FieldConvertors.h>

#include "Shared/Contract.h"
#include "Shared/EClient.h"
#include "ib/IBAPIValues.hpp"
#include "ib/ApiMessageBase.hpp"


using ib::internal::EClientPtr;


namespace IBAPI {
namespace V964 {

static const std::string& API_VERSION = "IBAPI964";

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
class MarketDataRequest : public IBAPI::ApiMessageBase
{
 public:

  static const std::string& MESSAGE_TYPE;

  MarketDataRequest():IBAPI::ApiMessageBase(API_VERSION, MESSAGE_TYPE)
  {
  }

  MarketDataRequest(const IBAPI::Message& copy) : IBAPI::ApiMessageBase(copy)
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
    if (securityType == IBAPI::SecurityType_OPTION) {

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


    } else if (securityType == IBAPI::SecurityType_COMMON_STOCK) {
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

    return true;
  }
};

const std::string& MarketDataRequest::MESSAGE_TYPE = "MarketDataRequest";





} // V964
} // IBAPI



#endif // IBAPI_API_MESSAGES_H_
