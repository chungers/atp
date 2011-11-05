#include "rcpp_hello_world.h"

#include "ib/ticker_id.hpp"

using namespace ib::internal;

SEXP rcpp_hello_world(){
  using namespace Rcpp ;

  CharacterVector x = CharacterVector::create( "hello", "raptor!!" )  ;
  NumericVector y   = NumericVector::create( 0.0, 1.0 ) ;
  List z            = List::create( x, y ) ;
  return z ;
}

SEXP raptor_ticker_id(){
  using namespace Rcpp ;

  CharacterVector x = CharacterVector::create( "hello", "raptor!!" )  ;
  NumericVector y   = NumericVector::create( 0.0, 1.0 ) ;
  List z            = List::create( x, y ) ;

  return z ;
}

