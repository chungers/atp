
#include <string>
#include <vector>
#include <zmq.hpp>

#include "ApiMessages.hpp"
#include "ib.h"

using namespace Rcpp ;

using std::string;

using boost::optional;
using boost::posix_time::ptime;
using boost::uint64_t;

using zmq::context_t;
using zmq::socket_t;


// IB API Version 9.66
using namespace IBAPI::V966;


SEXP api_connect(SEXP address)
{
  // create a zmq context
  context_t* context = new context_t(1);
  socket_t* socket;

  // gateway / api address:
  string zAddress = Rcpp::as<string>(address);

  bool status = true;
  try {
    socket = new socket_t(*context, ZMQ_REQ);
    socket->connect(zAddress.c_str());

    Rprintf("Connected to %s\n", zAddress.c_str());

  } catch (zmq::error_t e) {
    status = false;

    Rprintf("Failed to connected to %s\n", zAddress.c_str());
  }

  XPtr<context_t> contextPtr(context);
  XPtr<socket_t> socketPtr(socket);
  List result = List::create(Named("contextPtr", contextPtr),
                             Named("socketPtr", socketPtr),
                             Named("connected", wrap(status)));
  return result;
}


RcppExport SEXP ib_close(SEXP connectionHandle)
{
  List handleList(connectionHandle);
  XPtr<context_t> contextPtr(handleList["contextPtr"],
                             R_NilValue, R_NilValue);
  XPtr<socket_t> socketPtr(handleList["socketPtr"],
                             R_NilValue, R_NilValue);

  bool ok = as<bool>(handleList["connected"]);

  socketPtr.setDeleteFinalizer();
  contextPtr.setDeleteFinalizer();
  return wrap(ok);
}


#define R_STRING(x) Rcpp::as<std::string>(x)
#define R_BOOL(x) Rcpp::as<bool>(x)
#define R_INT(x) boost::lexical_cast<int>(R_STRING(x))
#define R_LONG(x) boost::lexical_cast<long>(R_STRING(x))
#define R_DOUBLE(x) boost::lexical_cast<double>(R_STRING(x))

bool operator>>(List& contractList, proto::ib::Contract* contract)
{
  contract->set_id(R_LONG(contractList["conId"]));
  contract->set_symbol(R_STRING(contractList["symbol"]));

  if (R_STRING(contractList["sectype"]) == "STK") {
    contract->set_type(proto::ib::Contract::STOCK);

  } else if (R_STRING(contractList["sectype"]) == "IND") {
    contract->set_type(proto::ib::Contract::INDEX);

  } else if (R_STRING(contractList["sectype"]) == "OPT") {
    contract->set_type(proto::ib::Contract::OPTION);

    if (R_STRING(contractList["right"]) == "P") {
      contract->set_right(proto::ib::Contract::PUT);
    } else if (R_STRING(contractList["right"]) == "C") {
      contract->set_right(proto::ib::Contract::CALL);
    }

    contract->mutable_strike()->set_amount(R_DOUBLE(contractList["strike"]));
    if (R_STRING(contractList["currency"]) == "USD") {
      contract->mutable_strike()->set_currency(proto::common::Money::USD);
    }
    contract->set_multiplier(R_INT(contractList["multiplier"]));

    int year, month, day;
    if (ParseDate(R_STRING(contractList["expiry"]), &year, &month, &day)) {
      contract->mutable_expiry()->set_year(year);
      contract->mutable_expiry()->set_month(month);
      contract->mutable_expiry()->set_day(day);
    }
  }

  if (R_STRING(contractList["exch"]).length() > 0) {
    contract->set_exchange(R_STRING(contractList["exch"]));
  }
  if (R_STRING(contractList["local"]).length() > 0) {
    contract->set_local_symbol(R_STRING(contractList["local"]));
  }

  // Do a test before we send it over the wire.  This is so that
  // we catch problems here instead of blowing up on the receiver side.
  Contract ibContract;
  return *contract >> ibContract;
}

template <typename P>
size_t send(P& message, SEXP connectionHandle)
{
  List handleList(connectionHandle);
  XPtr<zmq::socket_t> socket(handleList["socketPtr"], R_NilValue, R_NilValue);

  ib::internal::MessageId messageId = now_micros();
  return message.send(*socket, messageId);
}

int receive(SEXP connectionHandle)
{
  List handleList(connectionHandle);
  XPtr<zmq::socket_t> socket(handleList["socketPtr"], R_NilValue, R_NilValue);

  std::string buff;
  atp::zmq::receive(*socket, &buff);
  int response = boost::lexical_cast<int>(buff);
  switch (response) {
    case 200 :
      break;
    case 500 :
      break;
  }
  return response;
}

RcppExport SEXP api_request_marketdata(SEXP connectionHandle,
                                       SEXP contractList,
                                       SEXP tickTypesString,
                                       SEXP snapShotBool)
{
  IBAPI::V966::RequestMarketData req;
  proto::ib::Contract *contract = req.proto().mutable_contract();

  Rprintf("Created contract proto.");
  List rContractList(contractList);
  if (rContractList >> contract) {

    if (R_STRING(tickTypesString).length() > 0) {
      req.proto().set_tick_types(R_STRING(tickTypesString));
    }

    req.proto().set_snapshot(R_BOOL(snapShotBool));

    if (send<RequestMarketData>(req, connectionHandle) > 0) {
      return wrap(receive(connectionHandle));
    }
    return wrap(0);
  }
}


RcppExport SEXP api_cancel_marketdata(SEXP connectionHandle,
                                      SEXP contractList)
{
  IBAPI::V966::CancelMarketData req;
  proto::ib::Contract *contract = req.proto().mutable_contract();

  List rContractList(contractList);
  if (rContractList >> contract) {

    if (send<CancelMarketData>(req, connectionHandle) > 0) {
      return wrap(receive(connectionHandle));
    }
    return wrap(0);
  }
}
