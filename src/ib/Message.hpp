#ifndef IBAPI_MESSAGE_H_
#define IBAPI_MESSAGE_H_

#include "utils.hpp"


namespace IBAPI {

class Message
{
 public:

  virtual bool validate() = 0;

};

} // namespace IBAPI

#endif // IBAPI_MESSAGE_H_
