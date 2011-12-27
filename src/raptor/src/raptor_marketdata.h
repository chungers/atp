#ifndef _raptor_RAPTOR_MARKETDATA_H
#define _raptor_RAPTOR_MARKETDATA_H

#include <Rcpp.h>

/// Subscribe to market data for given list of topics which are
/// of the form AAPL.OPT.400.C.20111216
/// Handler is a callback function that processes the incoming data.
/// The handler is called with function arguments of source, timestamp,
/// event, data, latency.  Source is the handle to be used when unsubscribing
/// market data.
RcppExport SEXP firehose_subscribe_marketdata(SEXP endpoint,
                                              SEXP topics,
                                              SEXP handler);

/// Unsubscribe marketdata for the given contracts.
/// Handle is the rerturn valued from subscribe call.
/// Returns true / false to indicate success / failure.
RcppExport SEXP firehose_unsubscribe_marketdata(SEXP handle,
                                                SEXP topics);

#endif // _raptor_RAPTOR_MARKETDATA_H
