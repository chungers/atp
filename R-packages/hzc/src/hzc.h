#ifndef RHZC_H
#define RHZC_H

#include <Rcpp.h>


RcppExport SEXP hzc_connect(SEXP address, SEXP callbackAddr);

RcppExport SEXP hzc_close(SEXP connectionHandle);

RcppExport SEXP hzc_query(SEXP connectionHandle,
                          SEXP symbol, SEXP event,
                          SEXP rStart, SEXP rStop,
                          SEXP callback, SEXP est);


#endif // RHZC_H
