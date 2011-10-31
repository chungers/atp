#ifndef IBAPI_REACTOR_STRATEGY_FACTORY_H_
#define IBAPI_REACTOR_STRATEGY_FACTORY_H_

#include <zmq.hpp>

#include "ib/internal.hpp"
#include "ib/Message.hpp"

namespace ib {
namespace internal {

using IBAPI::Message;

class ReactorStrategy
{
 public:
  ~ReactorStrategy() {}

  virtual bool handleInboundMessage(Message& message, EClientPtr eclient) = 0;
};


class ReactorStrategyFactory
{

 private:
  ReactorStrategyFactory() {}
  ~ReactorStrategyFactory() {}

 public:
  static ReactorStrategy* getInstance();
};


} // internal
} // ib
#endif IBAPI_REACTOR_STRATEGY_FACTORY_H_
