#ifndef IB_INTERNAL_EVENT_DISPATCHER_BASE_
#define IB_INTERNAL_EVENT_DISPATCHER_BASE_

#include <sstream>
#include "common.hpp"
#include "marketdata_source.hpp"
#include "ib/EWrapperFactory.hpp"
#include "ib/TickerMap.hpp"
#include "ib/tick_types.hpp"
#include "varz/varz.hpp"
#include "zmq/ZmqUtils.hpp"

DEFINE_VARZ_int64(event_dispatch_publish_total_bytes, 0, "");
DEFINE_VARZ_int64(event_dispatch_publish_count, 0, "");
DEFINE_VARZ_int64(event_dispatch_publish_last_ts, 0, "");
DEFINE_VARZ_int64(event_dispatch_publish_interval_micros, 0, "");
DEFINE_VARZ_int64(event_dispatch_publish_unresolved_keys, 0, "");


DEFINE_VARZ_int64(event_dispatch_publish_depth_total_bytes, 0, "");
DEFINE_VARZ_int64(event_dispatch_publish_depth_count, 0, "");
DEFINE_VARZ_int64(event_dispatch_publish_depth_last_ts, 0, "");
DEFINE_VARZ_int64(event_dispatch_publish_depth_interval_micros, 0, "");
DEFINE_VARZ_int64(event_dispatch_publish_depth_unresolved_keys, 0, "");

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

      boost::uint64_t now = now_micros();
      atp::MarketData<T> marketData(topic, timed.getMicros(), tick, value);
      size_t sent = marketData.dispatch(getOutboundSocket(0));

      VARZ_event_dispatch_publish_count++;
      VARZ_event_dispatch_publish_total_bytes += sent;
      VARZ_event_dispatch_publish_interval_micros =
          now - VARZ_event_dispatch_publish_last_ts;
      VARZ_event_dispatch_publish_last_ts = now;

    } else {

      LOG(ERROR) << "Cannot get subscription key / topic for " << tickerId
                 << ", event = " << tickType << ", value " << value;

      VARZ_event_dispatch_publish_unresolved_keys++;

    }
  }

  void publishDepth(TickerId tickerId, int side, int level, int operation,
                    double price, int size,
                    TimeTracking& timed,
                    const std::string& mm = "L1")
  {
    std::string topic;
    if (TickerMap::getSubscriptionKeyFromId(tickerId, &topic)) {

      boost::uint64_t now = now_micros();

      atp::MarketDepth marketDepth(topic, timed.getMicros(),
                                   side, level, operation, price, size, mm);

      size_t sent = marketDepth.dispatch(getOutboundSocket(0));

      VARZ_event_dispatch_publish_depth_count++;
      VARZ_event_dispatch_publish_depth_total_bytes += sent;
      VARZ_event_dispatch_publish_depth_interval_micros =
          now - VARZ_event_dispatch_publish_depth_last_ts;
      VARZ_event_dispatch_publish_depth_last_ts = now;

    } else {

      LOG(ERROR) << "Cannot get subscription key / topic for " << tickerId
                 << ", market depth";

      VARZ_event_dispatch_publish_depth_unresolved_keys++;

    }
  }


 private:
  EWrapperEventCollector& eventCollector_;
};


} // internal
} // ib


#endif // IB_INTERNAL_EVENT_DISPATCHER_BASE_
