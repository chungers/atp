#ifndef R_IB_UTIL_H_
#define R_IB_UTIL_H_

#include <string>
#include <boost/lexical_cast.hpp>
#include <zmq.hpp>
#include <Rcpp.h>

#include "zmq/ZmqUtils.hpp"

// This is special parsing from string and cast to other scalar
// types.  This is because the Contract list in R has all the
// member properties as strings.


using namespace Rcpp ;

using zmq::context_t;
using zmq::socket_t;


#define R_STRING(x) Rcpp::as<string>(x)
#define R_BOOL(x) boost::lexical_cast<bool>(R_STRING(x))
#define R_INT(x) boost::lexical_cast<int>(R_STRING(x))
#define R_LONG(x) boost::lexical_cast<long>(R_STRING(x))
#define R_DOUBLE(x) boost::lexical_cast<double>(R_STRING(x))


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

#endif //R_IB_UTIL_H_
