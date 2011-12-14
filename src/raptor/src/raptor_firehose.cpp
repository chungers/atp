
#include <vector>

#include "ib/api964/ApiMessages.hpp"
#include "zmq/ZmqUtils.hpp"
#include "raptor_firehose.h"


using namespace std;
using namespace Rcpp ;


SEXP raptor_firehose_req_marketdata(SEXP handle, SEXP listKeys, SEXP list)
{
  List handleList(handle);
  XPtr<zmq::socket_t> socket(handleList["socket"], R_NilValue, R_NilValue);

  CharacterVector keys(listKeys);
  List rList(list);
  size_t fields = keys.size() - 1;
  size_t total = 0;
  for (CharacterVector::iterator key = keys.begin();
       key != keys.end();
       ++key, --fields) {
    std::string k(static_cast<const char*>(*key));
    std::string value = Rcpp::as<std::string>(rList[k]);
    std::string messageStr = k + "=" + value;
    size_t sent = atp::zmq::send_copy(*socket, messageStr, fields > 0);
    total += sent;
    Rprintf("Sent %d bytes, message = %s\n", sent, messageStr.c_str());
  }

  return wrap(total);
}
