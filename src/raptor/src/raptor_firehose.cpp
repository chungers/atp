
#include <vector>
#include <stdio.h>

#include "ib/api964/ApiMessages.hpp"
#include "zmq/ZmqUtils.hpp"
#include "raptor_firehose.h"


using namespace std;
using namespace Rcpp ;

using IBAPI::V964::CancelMarketDataRequest;
using IBAPI::V964::MarketDataRequest;

#define R_STRING Rcpp::as<std::string>


SEXP firehose_marketdata(SEXP handle, SEXP list)
{
  List handleList(handle);
  XPtr<zmq::socket_t> socket(handleList["socket"], R_NilValue, R_NilValue);

  List rList(list);

  MarketDataRequest mdr;

  mdr.set(FIX::MDEntryRefID(R_STRING(rList["conId"])));
  mdr.set(FIX::Symbol(R_STRING(rList["symbol"])));
  mdr.set(FIX::SecurityExchange(R_STRING(rList["exch"])));
  mdr.set(FIX::Currency(R_STRING(rList["currency"])));


  if (R_STRING(rList["sectype"]) == "STK") {
    mdr.set(FIX::SecurityType(FIX::SecurityType_COMMON_STOCK));
  } else if (R_STRING(rList["sectype"]) == "IND") {
    mdr.set(FIX::SecurityType(FIX::SecurityType_INDEXED_LINKED));
  } else if (R_STRING(rList["sectype"]) == "OPT") {
    mdr.set(FIX::SecurityType(FIX::SecurityType_OPTION));
    if (R_STRING(rList["right"]) == "P") {
      mdr.set(FIX::PutOrCall(FIX::PutOrCall_PUT));
    } else if (R_STRING(rList["right"]) == "C") {
      mdr.set(FIX::PutOrCall(FIX::PutOrCall_CALL));
    }
    float strike = 0;
    std::istringstream iss(R_STRING(rList["strike"]));
    iss >> strike;
    mdr.set(FIX::StrikePrice(strike));

    float multiplier = 0;
    std::istringstream iss2(R_STRING(rList["multiplier"]));
    iss2 >> multiplier;
    mdr.set(FIX::ContractMultiplier(multiplier));

    mdr.set(FIX::DerivativeSecurityID(R_STRING(rList["local"])));

    std::string expiry = R_STRING(rList["expiry"]);
    int year = 2011;
    int month = 12;
    int day = 16;

    if (sscanf(expiry.c_str(), "%4u%2u%2u", &year, &month, &day)) {
      IBAPI::V964::FormatExpiry(mdr, year, month, day);
    }
  }

  size_t sent = mdr.send(*socket);
  std::string buff;
  atp::zmq::receive(*socket, &buff);
  return wrap(buff);
}

SEXP firehose_cancel_marketdata(SEXP handle, SEXP list)
{
  List handleList(handle);
  XPtr<zmq::socket_t> socket(handleList["socket"], R_NilValue, R_NilValue);

  List rList(list);

  CancelMarketDataRequest cmdr;

  cmdr.set(FIX::MDEntryRefID(R_STRING(rList["conId"])));
  cmdr.set(FIX::Symbol(R_STRING(rList["symbol"])));
  cmdr.set(FIX::DerivativeSecurityID(R_STRING(rList["local"])));

  size_t sent = cmdr.send(*socket);
  std::string buff;
  atp::zmq::receive(*socket, &buff);
  return wrap(buff);
}
