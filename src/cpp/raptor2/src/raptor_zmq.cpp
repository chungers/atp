
#include <vector>

#include "zmq/ZmqUtils.hpp"
#include "raptor_zmq.h"


using namespace std;
using namespace Rcpp ;


SEXP raptor_zmq_connect(SEXP addr, SEXP socketType)
{
  std::string zmqAddr = Rcpp::as<std::string>(addr);
  std::string zmqSocketType = Rcpp::as<std::string>(socketType);

  zmq::context_t *context = new zmq::context_t(1);
  zmq::socket_t *socket;

  if (zmqSocketType == "ZMQ_REQ") {
    Rprintf("ZMQ_REP socket");
    socket = new zmq::socket_t(*context, ZMQ_REQ);
  } else if (zmqSocketType == "ZMQ_PUSH") {
    Rprintf("ZMQ_PUSH socket");
    socket = new zmq::socket_t(*context, ZMQ_PUSH);
  }

  socket->connect(zmqAddr.c_str());
  Rprintf(" connected @ %s\n", zmqAddr.c_str());

  XPtr<zmq::context_t> contextPtr(context);
  XPtr<zmq::socket_t> socketPtr(socket);

  List result = List::create(Named("context", contextPtr),
                             Named("socket", socketPtr),
                             Named("addr", zmqAddr),
                             Named("socket_type", zmqSocketType));
  return result;
}

SEXP raptor_zmq_disconnect(SEXP handle)
{
  List handleList(handle);
  XPtr<zmq::socket_t> socket(handleList["socket"], R_NilValue, R_NilValue);
  XPtr<zmq::context_t> context(handleList["context"], R_NilValue, R_NilValue);

  socket.setDeleteFinalizer();
  context.setDeleteFinalizer();
  return wrap(true);
}

SEXP raptor_zmq_send(SEXP handle, SEXP message)
{
  List handleList(handle);
  XPtr<zmq::socket_t> socket(handleList["socket"], R_NilValue, R_NilValue);

  std::string messageStr = Rcpp::as<std::string>(message);
  size_t sent = atp::zmq::send_copy(*socket, messageStr, false);
  Rprintf("Sent %d bytes, message = %s\n", sent, messageStr.c_str());

  return wrap(sent);
}

SEXP raptor_zmq_send_kv(SEXP handle, SEXP listKeys, SEXP list)
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
