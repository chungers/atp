#ifndef IBAPI_MESSAGE_H_
#define IBAPI_MESSAGE_H_

#include "ib/ib_common.hpp"

namespace IBAPI {

class Message {

 public:
  Message() : timestamp_(::now_micros()) {}
  ~Message() {}

  TimestampMicros when()
  {
    return timestamp_;
  }

 private:
  TimestampMicros timestamp_;
};


} // namespace IBAPI

#endif // IBAPI_MESSAGE_H_
