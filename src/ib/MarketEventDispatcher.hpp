#ifndef IB_INTERNAL_MARKET_EVENT_DISPATCHER_H_
#define IB_INTERNAL_MARKET_EVENT_DISPATCHER_H_

#include <string>
#include "common.hpp"
#include "log_levels.h"

#include "ib/TickerMap.hpp"
#include "ib/tick_types.hpp"

#include "proto/common.hpp"
#include "proto/ib.pb.h"
#include "historian/constants.hpp"

#include "varz/varz.hpp"
#include "zmq/ZmqUtils.hpp"

#include "ib/ApiEventDispatcher.hpp"


namespace ib {
namespace internal {


using proto::ib::MarketData;
using proto::ib::MarketDepth;



class MarketEventDispatcher : public IBAPI::ApiEventDispatcher
{
 public:
  MarketEventDispatcher(IBAPI::Application& app,
                        const IBAPI::SessionID& sessionId);

  ~MarketEventDispatcher();


  // Implementation in version specific impl directory.
  virtual EWrapper* GetEWrapper();

  template <typename T>
  void publish(TickerId tickerId, TickType tickType, const T& value,
               TimeTracking& timed)
  {
    boost::uint64_t now = now_micros();

    std::string tick = TickTypeNames[tickType];
    std::string topic;

    if (TickerMap::getSubscriptionKeyFromId(tickerId, &topic)) {

      MarketData ibMarketData;
      ibMarketData.set_symbol(topic);
      ibMarketData.set_timestamp(timed.getMicros());
      ibMarketData.set_event(tick);
      ibMarketData.set_contract_id(tickerId);
      proto::common::set_as(value, ibMarketData.mutable_value());

      // frames
      // 1. topic
      // 2. protobuff

      std::string proto;
      if (ibMarketData.SerializeToString(&proto)) {

        zmq::socket_t* socket = getOutboundSocket(0);

        size_t sent = atp::zmq::send_copy(*socket, topic, true);
        sent += atp::zmq::send_copy(*socket, proto, false);

        onPublish(now, sent);
      } else {

        LOG(ERROR) << "Unable to serialize: " << timed.getMicros()
                   << topic << ", event=" << tick << ", value=" << value;

        onSerializeError();
      }
    } else {

      LOG(ERROR) << "Cannot get subscription key / topic for " << tickerId
                 << ", event = " << tickType << ", value " << value;

      onUnresolvedTopic();
    }
    onCompletedPublishRequest(now);
  }

  void publishDepth(TickerId tickerId, int side, int level, int operation,
                    double price, int size,
                    TimeTracking& timed,
                    const std::string& mm = "L1");

 private:
  void onPublish(boost::uint64_t start, size_t sent);
  void onSerializeError();
  void onUnresolvedTopic();
  void onCompletedPublishRequest(boost::uint64_t start);
};


} // internal
} // ib


#endif // IB_INTERNAL_MARKET_EVENT_DISPATCHER_H_
