#ifndef RIB_H
#define RIB_H

#include <Rcpp.h>

RcppExport SEXP api_connect(SEXP address);

RcppExport SEXP api_close(SEXP connectionHandle);

RcppExport SEXP api_request_marketdata(SEXP connectionHandle,
                                       SEXP contractList,
                                       SEXP genericTicksString,
                                       SEXP snapShotBool);

RcppExport SEXP api_cancel_marketdata(SEXP connectionHandle,
                                      SEXP contractList);

RcppExport SEXP api_request_marketdepth(SEXP connectionHandle,
                                        SEXP contractList,
                                        SEXP rowsInt);

RcppExport SEXP api_cancel_marketdepth(SEXP connectionHandle,
                                       SEXP contractList);

#endif // RIB_H
