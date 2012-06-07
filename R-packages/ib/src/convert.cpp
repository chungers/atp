
#include <boost/lexical_cast.hpp>

#include "ApiMessages.hpp"
#include "convert.hpp"

using namespace Rcpp ;

using std::string;

namespace p = proto::ib;

#define R_STRING(x) Rcpp::as<string>(x)
#define R_BOOL(x) boost::lexical_cast<bool>(R_STRING(x))
#define R_INT(x) boost::lexical_cast<int>(R_STRING(x))
#define R_LONG(x) boost::lexical_cast<long>(R_STRING(x))
#define R_DOUBLE(x) boost::lexical_cast<double>(R_STRING(x))


bool operator>>(List& contractList, p::Contract* contract)
{
  contract->set_id(R_LONG(contractList["conId"]));
  contract->set_symbol(R_STRING(contractList["symbol"]));

  if (R_STRING(contractList["sectype"]) == "STK") {
    contract->set_type(p::Contract::STOCK);

  } else if (R_STRING(contractList["sectype"]) == "IND") {
    contract->set_type(p::Contract::INDEX);

  } else if (R_STRING(contractList["sectype"]) == "OPT") {
    contract->set_type(p::Contract::OPTION);

    if (R_STRING(contractList["right"]) == "P") {
      contract->set_right(p::Contract::PUT);
    } else if (R_STRING(contractList["right"]) == "C") {
      contract->set_right(p::Contract::CALL);
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


p::Contract* operator>>(List& orderList, p::Order* order)
{
  order->set_id(R_LONG(orderList["orderId"]));

  std::string orderRef = R_STRING(orderList["orderRef"]);
  if (orderRef.length() > 0) {
    order->set_ref(orderRef);
  }

  std::string action = R_STRING(orderList["action"]);
  if (action == "BUY") {
    order->set_action(p::Order::BUY);
  } else if (action == "SELL") {
    order->set_action(p::Order::SELL);
  } else if (action == "SSHORT") {
    order->set_action(p::Order::SSHORT);
  }


  order->set_quantity(R_INT(orderList["totalQuantity"]));

  if (R_INT(orderList["allOrNone"]) > 0) {
    order->set_all_or_none(true);
  }

  int minQty = R_INT(orderList["minQty"]);
  if (minQty > 0) {
    order->set_min_quantity(minQty);
  }

  if (R_INT(orderList["outsideRTH"]) > 0) {
    order->set_outside_rth(true);
  }

  std::string tif = R_STRING(orderList["tif"]);
  if (tif == "DAY") {
    order->set_time_in_force(p::Order::DAY);
  } else if (tif == "GTC") {
    order->set_time_in_force(p::Order::GTC);
  } else if (tif == "IOC") {
    order->set_time_in_force(p::Order::IOC);
  }

  return order->mutable_contract();
}
