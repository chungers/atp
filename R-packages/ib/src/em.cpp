
#include "ib/protocol.h"

#include "ApiMessages.hpp"

#include "convert.hpp"
#include "util.hpp"

#include "em.h"

using namespace Rcpp ;

using std::string;

using boost::optional;
using boost::posix_time::ptime;
using boost::uint64_t;


// IB API Version 9.66
namespace api = IBAPI::V966;


RcppExport SEXP api_place_order(SEXP connection,
                                SEXP orderList,
                                SEXP contractList)
{
  int code = -1;
  string resp("0");

  List rOrder(orderList);
  List rContract(contractList);

  std::string orderType = R_STRING(rOrder["orderType"]);
  if (orderType == "LMT") {

    api::LimitOrder req;
    proto::ib::Order *order = req.proto().mutable_base();

    Rprintf("LimitOrder\n");

    if (order << rOrder << rContract) {
      req.proto().mutable_limit_price()->set_amount(
          Rcpp::as<double>(rOrder["lmtPrice"]));
      code = DispatchMessage<api::LimitOrder>(connection, req, &resp);
    }

  } else if (orderType == "MKT") {

    api::MarketOrder req;
    proto::ib::Order *order = req.proto().mutable_base();

    Rprintf("MarketOrder\n");

    if (order << rOrder << rContract) {
      code = DispatchMessage<api::MarketOrder>(connection, req, &resp);
    }

  } else if (orderType == "STPLMT") {

    api::StopLimitOrder req;
    proto::ib::Order *order = req.proto().mutable_base();

    if (order << rOrder << rContract) {
      req.proto().mutable_limit_price()->set_amount(
          Rcpp::as<double>(rOrder["lmtPrice"]));
      req.proto().mutable_stop_price()->set_amount(
          Rcpp::as<double>(rOrder["auxPrice"]));
      code = DispatchMessage<api::StopLimitOrder>(connection, req, &resp);
    }
  }
  return List::create(Named("messageId",wrap(resp)), Named("code", wrap(code)));
}


RcppExport SEXP api_cancel_order(SEXP connection,
                                 SEXP orderId)
{
  int code = -1;
  string resp("0");

  api::CancelOrder req;

  req.proto().set_order_id(Rcpp::as<int>(orderId));
  code = DispatchMessage<api::CancelOrder>(connection, req, &resp);
  return List::create(Named("messageId",wrap(resp)), Named("code", wrap(code)));
}

