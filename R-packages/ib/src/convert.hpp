#ifndef RIB_CONVERT_H
#define RIB_CONVERT_H

#include <boost/cstdint.hpp>
#include <string>
#include <vector>
#include <zmq.hpp>

#include <Rcpp.h>

#include "ApiMessages.hpp"

using namespace Rcpp ;

using std::string;

using boost::optional;
using boost::posix_time::ptime;
using boost::uint64_t;

// This is special parsing from string and cast to other scalar
// types.  This is because the Contract list in R has all the
// member properties as strings.

#define R_STRING(x) Rcpp::as<string>(x)
#define R_BOOL(x) boost::lexical_cast<bool>(R_STRING(x))
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



#endif // RIB_CONVERT_H
