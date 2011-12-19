#ifndef _raptor_RAPTOR_FIREHOSE_H
#define _raptor_RAPTOR_FIREHOSE_H

#include <Rcpp.h>

RcppExport SEXP firehose_marketdata(SEXP handle, SEXP list) ;
RcppExport SEXP firehose_cancel_marketdata(SEXP handle, SEXP list) ;


#endif // _raptor_RAPTOR_FIREHOSE_H
