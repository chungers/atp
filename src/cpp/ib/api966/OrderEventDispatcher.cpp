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

#include "ib/OrderEventDispatcher.hpp"

#include "EventDispatcherEWrapperBase.hpp"



namespace ib {
namespace internal {


class OrderEventEWrapper :
      public EventDispatcherEWrapperBase<OrderEventDispatcher>
{
 public:

  explicit OrderEventEWrapper(IBAPI::Application& app,
                              const IBAPI::SessionID& sessionId,
                              OrderEventDispatcher& dispatcher) :
      EventDispatcherEWrapperBase<OrderEventDispatcher>(
          app, sessionId, dispatcher)
  {
  }

 public:


  /// @overload EWrapper
  void nextValidId(OrderId orderId)
  {
    LoggingEWrapper::nextValidId(orderId);
    LOG(INFO) << "Connection confirmed wth next order id = "
              << orderId;
    app_.onLogon(sessionId_);
  }

  /// @overload EWrapper
  void currentTime(long time)
  {
    LoggingEWrapper::currentTime(time);
    // TODO send some application admin message??
  }

  /// @Overload EWrapper
  void orderStatus(OrderId orderId, const IBString &status,
                   int filled,  int remaining,
                   double avgFillPrice,
                   int permId, int parentId,
                   double lastFillPrice, int clientId,
                   const IBString& whyHeld)
  {
    LoggingEWrapper::orderStatus(orderId, status, filled, remaining,
                                avgFillPrice, permId, parentId, lastFillPrice,
                                clientId, whyHeld);
    proto::ib::OrderStatus os;
    os.set_timestamp(now_micros());
    os.set_message_id(now_micros());
    os.set_order_id(orderId);
    os.set_status(status);
    os.set_filled(filled);
    os.set_remaining(remaining);
    os.mutable_avg_fill_price()->set_amount(avgFillPrice);
    os.mutable_last_fill_price()->set_amount(lastFillPrice);
    os.set_client_id(clientId);
    os.set_perm_id(permId);
    os.set_parent_id(parentId);
    os.set_why_held(whyHeld);

    dispatcher_.publish<proto::ib::OrderStatus>(os);
  }

};


EWrapper* OrderEventDispatcher::GetEWrapper()
{
  return new OrderEventEWrapper(Application(), SessionId(), *this);
}


} // internal
} // ib
