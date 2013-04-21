#ifndef _raptor_RAPTOR_HZC_H
#define _raptor_RAPTOR_HZC_H

#include <Rcpp.h>


RcppExport SEXP raptor_hzc_connect(SEXP address, SEXP callbackAddr);

RcppExport SEXP raptor_hzc_close(SEXP connectionHandle);

RcppExport SEXP raptor_hzc_query(SEXP connectionHandle,
                                 SEXP symbol, SEXP event,
                                 SEXP rStart, SEXP rStop,
                                 SEXP callback, SEXP est);


#endif // _raptor_RAPTOR_HZC_H
