#ifndef R_IB_H
#define R_IB_H

#include <Rcpp.h>

RcppExport SEXP api_connect(SEXP address);

RcppExport SEXP api_close(SEXP connectionHandle);

#endif // R_IB_H
