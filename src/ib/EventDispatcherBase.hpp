#ifndef IB_INTERNAL_EVENT_DISPATCHER_BASE_
#define IB_INTERNAL_EVENT_DISPATCHER_BASE_

#include "ib/EWrapperFactory.hpp"


namespace ib {
namespace internal {


class EventDispatcherBase
{
 public:
  EventDispatcherBase(EWrapperEventSink& eventSink) :
      eventSink_(eventSink)
  {
  }

  ~EventDispatcherBase() {}

 protected:

  zmq::socket_t* getEventSink()
  {
    return eventSink_.getSink();
  }

 private:
  EWrapperEventSink& eventSink_;
};


} // internal
} // ib


#endif // IB_INTERNAL_EVENT_DISPATCHER_BASE_
