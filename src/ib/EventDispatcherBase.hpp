#ifndef IB_INTERNAL_EVENT_DISPATCHER_BASE_
#define IB_INTERNAL_EVENT_DISPATCHER_BASE_

#include "ib/EWrapperFactory.hpp"


namespace ib {
namespace internal {


class EventDispatcherBase
{
 public:
  EventDispatcherBase(EWrapperEventCollector& eventCollector) :
      eventCollector_(eventCollector)
  {
  }

  ~EventDispatcherBase() {}

 protected:

  zmq::socket_t* getOutboundSocket(int channel = 0)
  {
    return eventCollector_.getOutboundSocket(channel);
  }

 private:
  EWrapperEventCollector& eventCollector_;
};


} // internal
} // ib


#endif // IB_INTERNAL_EVENT_DISPATCHER_BASE_
