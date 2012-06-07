
#include "ib/protocol.h"

#include "ApiMessages.hpp"

#include "convert.hpp"
#include "util.hpp"

#include "fh.h"

using namespace Rcpp ;

using std::string;

using boost::optional;
using boost::posix_time::ptime;
using boost::uint64_t;


// IB API Version 9.66
namespace api = IBAPI::V966;


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
