
#include <sstream>
#include "common.hpp"
#include "log_levels.h"

#include "ib/TickerMap.hpp"
#include "ib/tick_types.hpp"

#include "proto/common.hpp"
#include "proto/ib.pb.h"
#include "historian/constants.hpp"

#include "varz/varz.hpp"
#include "zmq/ZmqUtils.hpp"

#include "ib/OrderEventDispatcher.hpp"

DEFINE_VARZ_int64(em_event_dispatch_publish_total_bytes, 0, "");
DEFINE_VARZ_int64(em_event_dispatch_publish_count, 0, "");
DEFINE_VARZ_int64(em_event_dispatch_publish_last_ts, 0, "");
DEFINE_VARZ_int64(em_event_dispatch_publish_interval_micros, 0, "");
DEFINE_VARZ_int64(em_event_dispatch_publish_serialization_errors, 0, "");
DEFINE_VARZ_int64(em_event_dispatch_publish_micros, 0, "");


namespace ib {
namespace internal {


OrderEventDispatcher::OrderEventDispatcher(
    IBAPI::Application& app,
    const IBAPI::SessionID& sessionId) :
    IBAPI::ApiEventDispatcher(app, sessionId)
{
  VARZ_em_event_dispatch_publish_last_ts = now_micros();
}

OrderEventDispatcher::~OrderEventDispatcher() {}


void OrderEventDispatcher::onPublish(boost::uint64_t start, size_t sent)
{
  VARZ_em_event_dispatch_publish_count++;
  VARZ_em_event_dispatch_publish_total_bytes += sent;
  VARZ_em_event_dispatch_publish_interval_micros =
      start - VARZ_em_event_dispatch_publish_last_ts;
  VARZ_em_event_dispatch_publish_last_ts = start;
}

void OrderEventDispatcher::onSerializeError()
{
  VARZ_em_event_dispatch_publish_serialization_errors++;
}

void OrderEventDispatcher::onCompletedPublishRequest(boost::uint64_t start)
{
  VARZ_em_event_dispatch_publish_micros = now_micros() - start;
}


} // internal
} // ib

