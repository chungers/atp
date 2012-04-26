#ifndef MARKETDATA_H
#define MARKETDATA_H

#include <Rcpp.h>

/// Simply creates an instance of the market data subscriber
/// and returns its handle.
RcppExport SEXP mds_create_subscriber(SEXP id,
                                      SEXP adminEndpoint,
                                      SEXP eventEndpoint,
                                      SEXP endpoint,
                                      SEXP varzPort);

/// Subscribe to market data for given list of topics which are
/// of the form AAPL.OPT.400.C.20111216
/// subscriber is the returned value from _new() method.
RcppExport SEXP mds_subscribe(SEXP subscriber,
                              SEXP topics);

/// Unsubscribe marketdata for the given contracts.
/// Handle is the rerturn valued from subscribe call.
/// Returns true / false to indicate success / failure.
RcppExport SEXP mds_unsubscribe(SEXP subscriber,
                                SEXP topics);

/// Starts the subscriber.  This will block as the callback is being run.
RcppExport SEXP mds_start_subscriber(SEXP subscriber,
                                     SEXP handler);

/// Shutdown the subscriber.  This will unblock the run call.
RcppExport SEXP mds_stop_subscriber(SEXP subscriber);

#endif // MARKETDATA_H
