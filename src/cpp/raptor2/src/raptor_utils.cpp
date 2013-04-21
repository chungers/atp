#include <Rcpp.h>

#include "raptor_utils.h"
#include "ib/ticker_id.hpp"

using namespace std;
using namespace Rcpp ;
using namespace ib::internal;



SEXP raptor_to_ticker_id(SEXP symbol){
  std::string s = Rcpp::as<std::string>(symbol);
  int tickerId = SymbolToTickerId(s);
  Rprintf("Symbol = %s, TickerId = %d\n", s.c_str(), tickerId);

  IntegerVector result(1);
  result[0] = tickerId;
  return result;
}

SEXP raptor_to_symbol(SEXP tickerId){
  int code = Rcpp::as<int>(tickerId);
  std::string symbol;
  SymbolFromTickerId(code, &symbol);

  Rprintf("TickerId = %d, Symbol = %s\n", code, symbol.c_str());
  CharacterVector result = CharacterVector::create(symbol);
  return result;
}


