#ifndef _raptor_RAPTOR_UTILS_H
#define _raptor_RAPTOR_UTILS_H

#include <Rcpp.h>


RcppExport SEXP raptor_to_ticker_id(SEXP symbol) ;
RcppExport SEXP raptor_to_symbol(SEXP tickerId) ;


#endif // _raptor_RAPTOR_UTILS_H
