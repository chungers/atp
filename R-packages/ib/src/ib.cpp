
#include "ib/protocol.h"

#include "ApiMessages.hpp"

#include "convert.hpp"
#include "ib.h"

using namespace Rcpp ;

using std::string;

using boost::optional;
using boost::posix_time::ptime;
using boost::uint64_t;

using zmq::context_t;
using zmq::socket_t;


// IB API Version 9.66
namespace api = IBAPI::V966;


template<typename P>
unsigned int DispatchMessage(SEXP connection, P& req, string* out)
{
  List handleList(connection);
  XPtr<zmq::socket_t> socket(handleList["socketPtr"], R_NilValue, R_NilValue);

  ib::internal::MessageId messageId = now_micros();
  out->assign(boost::lexical_cast<string>(messageId));

  size_t sent = req.send(*socket, messageId);

  int responseCode = 0;

#ifdef SOCKET_CONNECTOR_USES_BLOCKING_REACTOR

  string buff;
  atp::zmq::receive(*socket, &buff);
  int response = boost::lexical_cast<int>(buff);

  switch (response) {
    case 200 :
      break;

    default:
      Rprintf("Error: %d\n", response);
      break;
  }
#endif

  return responseCode;
}

SEXP api_connect(SEXP address)
{
  // create a zmq context
  context_t* context = new context_t(1);
  socket_t* socket;

  // gateway / api address:
  string zAddress = Rcpp::as<string>(address);

  bool status = true;
  try {
    socket = new socket_t(*context, ZMQ_PUSH);
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


RcppExport SEXP ib_close(SEXP connection)
{
  List handleList(connection);
  XPtr<context_t> contextPtr(handleList["contextPtr"],
                             R_NilValue, R_NilValue);
  XPtr<socket_t> socketPtr(handleList["socketPtr"],
                             R_NilValue, R_NilValue);

  bool ok = as<bool>(handleList["connected"]);

  socketPtr.setDeleteFinalizer();
  contextPtr.setDeleteFinalizer();
  return wrap(ok);
}



RcppExport SEXP api_request_marketdata(SEXP connection,
                                       SEXP contractList,
                                       SEXP tickTypesString,
                                       SEXP snapShotBool)
{
  unsigned int code = 0;
  string resp("0");

  api::RequestMarketData req;
  proto::ib::Contract *contract = req.proto().mutable_contract();

  List rContractList(contractList);
  if (rContractList >> contract) {

    if (R_STRING(tickTypesString).length() > 0) {
      req.proto().set_tick_types(R_STRING(tickTypesString));
    }

    bool snapshot = Rcpp::as<bool>(snapShotBool);
    req.proto().set_snapshot(snapshot);

    code = DispatchMessage<api::RequestMarketData>(connection, req, &resp);
  }
  return List::create(Named("messageId",wrap(resp)), Named("code", wrap(code)));
}


RcppExport SEXP api_cancel_marketdata(SEXP connection,
                                      SEXP contractList)
{
  unsigned int code = 0;
  string resp("0");

  api::CancelMarketData req;
  proto::ib::Contract *contract = req.proto().mutable_contract();

  List rContractList(contractList);
  if (rContractList >> contract) {
    code = DispatchMessage<api::CancelMarketData>(connection, req, &resp);
  }
  return List::create(Named("messageId",wrap(resp)), Named("code", wrap(code)));
}

RcppExport SEXP api_request_marketdepth(SEXP connection,
                                        SEXP contractList,
                                        SEXP rowsInt)
{
  unsigned int code = 0;
  string resp("0");

  api::RequestMarketDepth req;
  proto::ib::Contract *contract = req.proto().mutable_contract();

  List rContractList(contractList);
  if (rContractList >> contract) {

    int rows = Rcpp::as<int>(rowsInt);
    if (rows > 0) {
      req.proto().set_rows(rows);
    }

    code = DispatchMessage<api::RequestMarketDepth>(connection, req, &resp);
  }
  return List::create(Named("messageId",wrap(resp)), Named("code", wrap(code)));
}


RcppExport SEXP api_cancel_marketdepth(SEXP connection,
                                       SEXP contractList)
{
  unsigned int code = 0;
  string resp("0");

  api::CancelMarketDepth req;
  proto::ib::Contract *contract = req.proto().mutable_contract();

  List rContractList(contractList);
  if (rContractList >> contract) {
    code = DispatchMessage<api::CancelMarketDepth>(connection, req, &resp);
  }
  return List::create(Named("messageId",wrap(resp)), Named("code", wrap(code)));
}
