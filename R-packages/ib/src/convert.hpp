#ifndef R_IB_CONVERT_H
#define R_IB_CONVERT_H

#include <boost/cstdint.hpp>
#include <string>

#include <Rcpp.h>

#include "proto/ib.pb.h"


using namespace Rcpp ;


namespace p = proto::ib;

bool operator>>(List& contractList, p::Contract* contract);
inline bool operator<<(p::Contract* contract, List& contractList)
{
  return contractList >> contract;
}


p::Contract* operator>>(List& orderList, p::Order* order);
inline p::Contract* operator<<(p::Order* order, List& orderList)
{
  return orderList >> order;
}



#endif // R_IB_CONVERT_H
