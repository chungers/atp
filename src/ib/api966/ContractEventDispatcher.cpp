#include <sstream>
#include <glog/logging.h>

#include "common.hpp"
#include "log_levels.h"

#include "varz/varz.hpp"

#include "ib/Application.hpp"
#include "ib/SessionID.hpp"
#include "ib/Exceptions.hpp"
#include "ib/Message.hpp"
#include "ApiImpl.hpp"
#include "ib/ContractEventDispatcher.hpp"

#include "EventDispatcherEWrapperBase.hpp"

#include "marshall.hpp"
#include "ostream.hpp" // for printing contract

namespace ib {
namespace internal {


class ContractEventEWrapper :
      public EventDispatcherEWrapperBase<ContractEventDispatcher>
{
 public:

  explicit ContractEventEWrapper(IBAPI::Application& app,
                                 const IBAPI::SessionID& sessionId,
                                 ContractEventDispatcher& dispatcher) :
      EventDispatcherEWrapperBase<ContractEventDispatcher>(
          app, sessionId, dispatcher)
  {
  }

  /// @overload EWrapper
  void contractDetails(int reqId,
                       const ContractDetails& contractDetails) {
    LoggingEWrapper::contractDetails(reqId, contractDetails);

    proto::ib::ContractDetailsResponse resp;

    boost::uint64_t now = now_micros();
    resp.set_timestamp(now);
    resp.set_message_id(now);

    if (*(resp.mutable_details()) << contractDetails) {
      dispatcher_.publishContractEvent<proto::ib::ContractDetailsResponse>(
          reqId, resp);

    } else {
      LOG(ERROR) << "Unable to marshall contract details: "
                 << contractDetails.summary;
    }
  }

  /// @overload EWrapper
  void contractDetailsEnd(int reqId) {
    LoggingEWrapper::contractDetailsEnd(reqId);

    proto::ib::ContractDetailsEnd detailsEnd;

    boost::uint64_t now = now_micros();
    detailsEnd.set_timestamp(now);
    detailsEnd.set_message_id(now);
    detailsEnd.set_request_id(reqId);

    dispatcher_.publishContractEvent<proto::ib::ContractDetailsEnd>(
        reqId, detailsEnd);
  }

  /// @overload base
  virtual void onNoContractDefinition(int error_code, int req_id) {
    LOG(ERROR) << "No contract definition for request " << req_id
               << ", sending simulated contractDetailsEnd.";

    contractDetailsEnd(req_id);
  }

};


EWrapper* ContractEventDispatcher::GetEWrapper()
{
  return new ContractEventEWrapper(Application(), SessionId(), *this);
}


} // internal
} // ib
