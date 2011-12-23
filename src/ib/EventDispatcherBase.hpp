#ifndef IB_INTERNAL_EVENT_DISPATCHER_BASE_
#define IB_INTERNAL_EVENT_DISPATCHER_BASE_

#include <sstream>
#include "common.hpp"
#include "marketdata.hpp"
#include "ib/EWrapperFactory.hpp"
#include "ib/TickerMap.hpp"
#include "ib/tick_types.hpp"
#include "zmq/ZmqUtils.hpp"

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

  template <typename T>
  void publish(TickerId tickerId, TickType tickType, const T& value,
               TimeTracking& timed)
  {
    std::string tick = TickTypeNames[tickType];
    std::string topic;

    if (TickerMap::getSubscriptionKeyFromId(tickerId, &topic)) {

      atp::MarketData<T> marketData(topic, timed.getMicros(), tick, value);
      size_t sent = marketData.dispatch(getOutboundSocket(0));

    } else {
      LOG(ERROR) << "Cannot get subscription key / topic for " << tickerId
                 << ", event = " << tickType << ", value " << value;
    }
  }



 private:
  EWrapperEventCollector& eventCollector_;
};


} // internal
} // ib


#endif // IB_INTERNAL_EVENT_DISPATCHER_BASE_
