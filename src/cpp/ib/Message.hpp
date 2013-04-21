#ifndef IBAPI_MESSAGE_H_
#define IBAPI_MESSAGE_H_

#include <boost/cstdint.hpp>
#include "utils.hpp"


namespace IBAPI {

class Message
{
 public:

  /**
   * key is the message type / key, e.g., 'LimitOrder'
   */
  virtual const std::string key() const = 0;


  virtual bool validate() = 0;

};

} // namespace IBAPI

#endif // IBAPI_MESSAGE_H_
