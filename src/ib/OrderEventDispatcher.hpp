#ifndef IB_INTERNAL_ORDER_EVENT_DISPATCHER_H_
#define IB_INTERNAL_ORDER_EVENT_DISPATCHER_H_

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


class OrderEventDispatcher : public IBAPI::ApiEventDispatcher
{
 public:
  OrderEventDispatcher(IBAPI::Application& app,
                       const IBAPI::SessionID& sessionId);

  ~OrderEventDispatcher();


  // Implementation in version specific impl directory.
  virtual EWrapper* GetEWrapper();

  template <typename T>
  void publish(T& proto)
  {
    boost::uint64_t now = now_micros();
    // frames
    // 1. message_key
    // 2. protobuff
    std::string frame;
    if (proto.SerializeToString(&frame)) {

        zmq::socket_t* socket = getOutboundSocket(0);
        assert(socket != NULL);

        size_t sent = atp::zmq::send_copy(*socket, proto.key(), true);
        sent += atp::zmq::send_copy(*socket, frame, false);

        onPublish(now, sent);

    } else {
      LOG(ERROR) << "Unable to serialize: " << proto.key()
                 << ", message_id=" << proto.message_id()
                 << ", ts=" << proto.timestamp();

      onSerializeError();
    }
    onCompletedPublishRequest(now);
  }

 private:
  void onPublish(boost::uint64_t start, size_t sent);
  void onSerializeError();
  void onCompletedPublishRequest(boost::uint64_t start);
};


} // internal
} // ib


#endif // IB_INTERNAL_ORDER_EVENT_DISPATCHER_H_
