#ifndef IBAPI_API_MESSAGES_H_
#define IBAPI_API_MESSAGES_H_

#include "Shared/Contract.h"
#include "Shared/EClient.h"
#include "ib/ApiMessageBase.hpp"


using ib::internal::EClientPtr;

namespace IBAPI {
namespace V964 {

static const std::string& MESSAGE_VERSION = "IBAPI964";

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

  static const std::string& MESSAGE_TYPE_NAME;

  MarketDataRequest() :
      IBAPI::ApiMessageBase(MESSAGE_VERSION, MESSAGE_TYPE_NAME)
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


  bool callApi(EClientPtr eclient)
  {
    FIX::Symbol symbol;  get(symbol);
    FIX::MDEntryRefID conId; get(conId);
    FIX::SecurityID securityId; get(securityId);
    FIX::SecurityType securityType; get(securityType);
    FIX::DerivativeSecurityID localSymbol; get(localSymbol);
    FIX::SecurityExchange exchange; get(exchange);
    FIX::PutOrCall putOrCall; get(putOrCall);
    FIX::StrikePrice strike; get(strike);
    FIX::Currency currency; get(currency);
    FIX::ContractMultiplier contractMultiplier; get(contractMultiplier);
    FIX::MaturityMonthYear expiryMonthYear; get(expiryMonthYear);
    FIX::MaturityDay expiryDay; get(expiryDay);


    // TWS Contract
    Contract contract;
    contract.symbol = symbol;
    contract.localSymbol = localSymbol;
    contract.exchange = exchange;
    contract.strike = strike;
    contract.currency = currency;
    contract.multiplier = contractMultiplier;

    if (securityType == FIX::SecurityType_COMMON_STOCK) {
      contract.secType= "STK";
    } else if (securityType == FIX::SecurityType_OPTION) {
      contract.secType = "OPT";
    }

    switch (putOrCall) {
      case FIX::PutOrCall_PUT:
        contract.right = "P";
        break;
      case FIX::PutOrCall_CALL:
        contract.right = "C";
        break;
    }

    // TODO expiry

    // long
    //contract.conId =

    beforeApiCall(contract, eclient);

  }

  virtual void beforeApiCall(Contract& contract, EClientPtr eclient)
  {
    // NO-OP -- for testing.
  }
};

const std::string& MarketDataRequest::MESSAGE_TYPE_NAME = "MarketDataRequest";





} // V964
} // IBAPI



#endif // IBAPI_API_MESSAGES_H_
