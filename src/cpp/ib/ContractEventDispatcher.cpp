
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

#include "ib/ContractEventDispatcher.hpp"

DEFINE_VARZ_int64(cm_event_dispatch_publish_total_bytes, 0, "");
DEFINE_VARZ_int64(cm_event_dispatch_publish_count, 0, "");
DEFINE_VARZ_int64(cm_event_dispatch_publish_last_ts, 0, "");
DEFINE_VARZ_int64(cm_event_dispatch_publish_interval_micros, 0, "");
DEFINE_VARZ_int64(cm_event_dispatch_publish_serialization_errors, 0, "");
DEFINE_VARZ_int64(cm_event_dispatch_publish_micros, 0, "");


namespace ib {
namespace internal {


ContractEventDispatcher::ContractEventDispatcher(
    IBAPI::Application& app,
    const IBAPI::SessionID& sessionId) :
    IBAPI::ApiEventDispatcher(app, sessionId)
{
  VARZ_cm_event_dispatch_publish_last_ts = now_micros();
}

ContractEventDispatcher::~ContractEventDispatcher() {}


void ContractEventDispatcher::onPublish(boost::uint64_t start, size_t sent)
{
  VARZ_cm_event_dispatch_publish_count++;
  VARZ_cm_event_dispatch_publish_total_bytes += sent;
  VARZ_cm_event_dispatch_publish_interval_micros =
      start - VARZ_cm_event_dispatch_publish_last_ts;
  VARZ_cm_event_dispatch_publish_last_ts = start;
}

void ContractEventDispatcher::onSerializeError()
{
  VARZ_cm_event_dispatch_publish_serialization_errors++;
}

void ContractEventDispatcher::onCompletedPublishRequest(boost::uint64_t start)
{
  VARZ_cm_event_dispatch_publish_micros = now_micros() - start;
}


} // internal
} // ib

