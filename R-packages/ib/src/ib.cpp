
#include <zmq.hpp>
#include "ib/protocol.h"
#include "ib.h"

using namespace Rcpp ;

using std::string;

using zmq::context_t;
using zmq::socket_t;


RcppExport SEXP api_connect(SEXP address)
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


