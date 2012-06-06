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


//class OrderEventEWrapper : public LoggingEWrapper, NoCopyAndAssign
class OrderEventEWrapper :
      public EventDispatcherEWrapperBase<OrderEventDispatcher>
{
 public:

  explicit OrderEventEWrapper(IBAPI::Application& app,
                              const IBAPI::SessionID& sessionId,
                              OrderEventDispatcher& dispatcher) :
      EventDispatcherEWrapperBase<OrderEventDispatcher>(app, sessionId, dispatcher)
  {
  }

 //      app_(app),
 //      sessionId_(sessionId),
 //      dispatcher_(dispatcher)
 //  {
 //  }

 // private:
 //  IBAPI::Application& app_;
 //  const IBAPI::SessionID& sessionId_;
 //  OrderEventDispatcher& dispatcher_;

 public:

  /// @overload EWrapper
  /*
  void error(const int id, const int errorCode, const IBString errorString)
  {
    LoggingEWrapper::error(id, errorCode, errorString);
    LOG(ERROR) << "ERROR(" << errorCode << "): " << errorString;

    std::ostringstream msg;
    msg << "IBAPI_ERROR[" << errorCode << "]: " << errorString
        << " -- ";

    bool terminate = false;

    switch (errorCode) {

      // The following code will cause exception to be thrown, which
      // will force the event loop to terminate.
      case 200:
        terminate = false;
        VARZ_ib_api_oe_error_no_security_definition++;
        msg << "No security definition has been found for the request, id="
            << id;
        break;
      case 326:
        terminate = true;
        msg << "Conflicting connection id. Disconnecting.";
        break;
      case 502:
        terminate = false; // Set to false to avoid hanging connection on linux
        VARZ_ib_api_oe_error_cannot_connect++;
        msg << "Couldn't connect to TWS.  "
            << "Confirm that \"Enable ActiveX and Socket Clients\" is enabled on the TWS "
            << "\"Configure->API\" menu.";
        break;
      case 509:
        terminate = false; // Set to false to avoid hanging connection on linux
        VARZ_ib_api_oe_error_connection_reset++;
        msg << "Connection reset. Disconnecting.";
        break;
      case 1100:
        LOG(ERROR) << "Error 1100 -- not disconnecting. Expect IBGateway to reconnect.";
        terminate = false;
        VARZ_ib_api_oe_error_gateway_disconnect++;
        msg << "No disconnect.";
        break;
      case 2103:
        terminate = false;
        VARZ_ib_api_oe_error_marketdata_farm_connection_broken++;
        msg << "Market data farm connection is broken.";
        break;
      case 2104:
        terminate = false;
        VARZ_ib_api_oe_error_marketdata_farm_connection_ok++;
        msg << "Market data farm connection is OK";
        break;
      case 2110:
        terminate = false;
        VARZ_ib_api_oe_error_gateway_server_connection_broken++;
        msg << "Connectivity between TWS and server is broken. It will be restored automatically.";
        break;

      default:
        terminate = false;
        VARZ_ib_api_oe_error_unhandled++;
        msg << "Unhandled error.  Continue...";
        break;
    }

    LOG(WARNING) << msg.str();
    if (terminate) {
        throw IBAPI::RuntimeError(msg.str());
    }
  }
  */

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
