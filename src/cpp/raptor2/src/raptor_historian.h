#ifndef _raptor_RAPTOR_HISTORIAN_H
#define _raptor_RAPTOR_HISTORIAN_H

#include <Rcpp.h>


RcppExport SEXP raptor_historian_open(SEXP db);

RcppExport SEXP raptor_historian_close(SEXP dbHandle);

RcppExport SEXP raptor_historian_ib_marketdata(SEXP dbHandle,
                                               SEXP symbol, SEXP event,
                                               SEXP rStart, SEXP rStop,
                                               SEXP callback,
                                               SEXP est);

RcppExport SEXP raptor_historian_sessionlogs(SEXP dbHandle, SEXP symbol);


#endif // _raptor_RAPTOR_HISTORIAN_H
