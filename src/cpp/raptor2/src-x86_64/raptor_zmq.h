#ifndef _raptor_RAPTOR_ZMQ_H
#define _raptor_RAPTOR_ZMQ_H

#include <Rcpp.h>


RcppExport SEXP raptor_zmq_connect(SEXP addr, SEXP type) ;
RcppExport SEXP raptor_zmq_disconnect(SEXP handle);
RcppExport SEXP raptor_zmq_send(SEXP handle, SEXP message) ;
RcppExport SEXP raptor_zmq_send_kv(SEXP handle, SEXP keys, SEXP list) ;


#endif // _raptor_RAPTOR_ZMQ_H
