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

    if (tickerMap_.getSubscriptionKeyFromId(tickerId, &topic)) {

      atp::MarketData<T> marketData(topic, timed.getMicros(), tick, value);
      size_t sent = marketData.dispatch(getOutboundSocket(0));

      // zmq::socket_t* out = getOutboundSocket(0);
      // if (out) {

      //   boost::uint64_t t = timed.getMicros();
      //   std::ostringstream ts;
      //   ts << t;

      //   std::ostringstream nv;
      //   nv << tick << '=' << value;

      //   std::ostringstream latency;
      //   latency << now_micros() - t;

      //   atp::zmq::send_copy(*out, topic, true);
      //   atp::zmq::send_copy(*out, ts.str(), true);
      //   atp::zmq::send_copy(*out, nv.str(), true);
      //   atp::zmq::send_copy(*out, latency.str(), false);
      // }
    } else {
      LOG(ERROR) << "Cannot get subscription key / topic for " << tickerId
                 << ", event = " << tickType << ", value " << value;
    }
  }



 private:
  EWrapperEventCollector& eventCollector_;
  TickerMap tickerMap_;
};


} // internal
} // ib


#endif // IB_INTERNAL_EVENT_DISPATCHER_BASE_
