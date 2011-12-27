#ifndef _raptor_RAPTOR_FIREHOSE_H
#define _raptor_RAPTOR_FIREHOSE_H

#include <Rcpp.h>

/// Request for market data given the contract
RcppExport SEXP firehose_marketdata(SEXP handle, SEXP list);

/// Request to cancel market data for given contract
RcppExport SEXP firehose_cancel_marketdata(SEXP handle, SEXP list);


#endif // _raptor_RAPTOR_FIREHOSE_H
