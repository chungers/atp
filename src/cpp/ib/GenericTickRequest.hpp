#ifndef IB_INTERNAL_GENERIC_TICK_REQUEST_H_
#define IB_INTERNAL_GENERIC_TICK_REQUEST_H_

#include <iostream>
#include <string>
#include <vector>

#include "ib/tick_types.hpp"

namespace ib {
namespace internal {

/**
   IB API's Generic Tick Request builder.
   This class builds a string that encodes the generic tick requests per
   IB's API doc (see http://goo.gl/LKfOD), for use when calling the API
   to request market data.
 */
class GenericTickRequest {

 public:
  GenericTickRequest() {
  }

  ~GenericTickRequest() {}

  GenericTickRequest& add(const GenericTickType& type) {
    requestTypes_.push_back(type);
    return *this;
  }

  /**
     Builds the request string per IB's spec -- as a comma-delimited string
     of code values.
  */
  std::string toString() {
    std::ostringstream oss;
    for (unsigned int i = 0; i < requestTypes_.size(); ++i) {
      if (i > 0) {
        oss << "," << requestTypes_[i];
      } else {
        oss << requestTypes_[i];
      }
    }
    return oss.str();
  }
  
 private:
  std::vector<GenericTickType> requestTypes_;

};


} // internal
} // ib




#endif // IB_INTERNAL_GENERIC_TICK_REQUEST_H_
