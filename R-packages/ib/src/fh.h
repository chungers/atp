#ifndef R_IB_FH_H
#define R_IB_FH_H

#include <Rcpp.h>


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

#endif // R_IB_FH_H
