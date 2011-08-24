#ifndef IB_EVENT_DISPATCHER_H_
#define IB_EVENT_DISPATCHER_H_

#include <sstream>
#include <glog/logging.h>

#include "ib/Application.hpp"
#include "ib/Exceptions.hpp"
#include "ib/Message.hpp"
#include "ApiImpl.hpp"
#include "ib/ticker_id.hpp"

namespace ib {
namespace internal {

//class SocketConnector::Stragtegy;

/**
 * Basic design follows SocketConnector / SocketInitiator in QuickFIX.
 *
 * See https://github.com/lab616/third_party/blob/master/quickfix-1.13.3/src/C++/SocketConnector.h
 */
class EventDispatcher : public LoggingEWrapper {

 public:

  EventDispatcher(IBAPI::Application& app, int clientId)
      : app_(app)
      , clientId_(clientId)
  {
  }

 private:
  IBAPI::Application& app_;
  int clientId_;

 public:

  /// @overload EWrapper
  void error(const int id, const int errorCode, const IBString errorString)
  {

    LoggingEWrapper::error(id, errorCode, errorString);
    std::ostringstream msg;
    msg << "IBAPI_ERROR[" << errorCode << "]: ";

    bool terminate = false;
    std::string symbol;
    SymbolFromTickerId(id, &symbol);

    switch (errorCode) {

      // The following code will cause exception to be thrown, which
      // will force the event loop to terminate.
      case 200:
        terminate = false;
        msg << "No security definition has been found for the request, symbol="
            << symbol;
        break;
      case 326:
        terminate = true;
        msg << "Conflicting connection id. Disconnecting.";
        break;
      case 502:
        terminate = true;
        msg << "Couldn't connect to TWS.  "
            << "Confirm that \"Enable ActiveX and Socket Clients\" is enabled on the TWS "
            << "\"Configure->API\" menu.";
        break;
      case 509:
        terminate = true;
        msg << "Connection reset. Disconnecting.";
        break;
      case 1100:
        terminate = true;
        msg << "Disconnecting.";
        break;
      case 2103:
        terminate = false;
        msg << "Market data farm connection is broken.";
        break;
      case 2104:
        terminate = false;
        msg << "Market data farm connection is OK";
        break;
      case 2110:
        terminate = false;
        msg << "Connectivity between TWS and server is broken. It will be restored automatically.";
        break;

      default:
        terminate = false;
        msg << "Unhandled error.  Continue...";
        break;
    }
    
    LOG(WARNING) << msg.str();
    if (terminate) {
        throw IBAPI::RuntimeError(msg.str());
    }
  }

  /// @overload EWrapper
  void nextValidId(OrderId orderId)
  {
    LoggingEWrapper::nextValidId(orderId);
    LOG(INFO) << "Connection confirmed wth next order id = "
              << orderId;
    app_.onLogon(clientId_);
    //IBAPI::NextOrderIdMessage m(orderId);
    //app_.fromAdmin(m, get_connection_id());
  }

  /// @overload EWrapper
  void currentTime(long time)
  {
    LoggingEWrapper::currentTime(time);

    //IBAPI::HeartBeatMessage m;
    //m.currentTime = time;
    //app_.fromAdmin(m, get_connection_id());
  }


};


} // internal
} // ib

#endif // IB_EVENT_DISPATCHER_H_
