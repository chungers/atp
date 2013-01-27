#ifndef IB_INTERNAL_CONTRACT_EVENT_DISPATCHER_H_
#define IB_INTERNAL_CONTRACT_EVENT_DISPATCHER_H_

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


using std::string;


namespace ib {
namespace internal {


class ContractEventDispatcher : virtual public IBAPI::ApiEventDispatcher
{
 public:
  ContractEventDispatcher(IBAPI::Application& app,
                          const IBAPI::SessionID& sessionId);

  ~ContractEventDispatcher();


  // Implementation in version specific impl directory.
  virtual EWrapper* GetEWrapper();

  template <typename Proto>
  void publishContractEvent(int requestId, const Proto& event)
  {
    UNUSED(requestId);

    boost::uint64_t now = now_micros();

    string buff;
    if (event.SerializeToString(&buff)) {
      ::zmq::socket_t* socket = getOutboundSocket(0);
      size_t sent = atp::zmq::send_copy(*socket, event.GetTypeName(), true);
      sent += atp::zmq::send_copy(*socket, buff, false);

      CONTRACT_MANAGER_LOGGER << "Sent (" << sent << "): "
                              << event.GetTypeName();

      onPublish(now, sent);

    } else {

      CONTRACT_MANAGER_ERROR << "Unable to serialize: " << event.GetTypeName();

      onSerializeError();
    }

    onCompletedPublishRequest(now);
  }

 private:
  void onPublish(boost::uint64_t start, size_t sent);
  void onSerializeError();
  void onUnresolvedTopic();
  void onCompletedPublishRequest(boost::uint64_t start);
};


} // internal
} // ib


#endif // IB_INTERNAL_CONTRACT_EVENT_DISPATCHER_H_
