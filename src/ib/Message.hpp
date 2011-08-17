#ifndef IBAPI_MESSAGES_H_
#define IBAPI_MESSAGES_H_

#include <cctype>
#include "utils.hpp"

namespace IBAPI {

typedef uint64_t timestamp;

class Message {

 public:
  Message() : timestamp_(::now_micros()) {}
  
  ~Message() {}

  timestamp when() { return timestamp_; }
  
 private:
  timestamp timestamp_;
};


} // namespace IBAPI

#endif // IBAPI_MESSAGES_H_
