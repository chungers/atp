#ifndef IB_EVENT_DISPATCHER_H_
#define IB_EVENT_DISPATCHER_H_

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "ib/Application.hpp"
#include "ib/Message.hpp"
#include "ib/SocketConnector.hpp"
#include "ApiImpl.hpp"

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

  EventDispatcher(IBAPI::Application& app, IBAPI::SocketConnector::Strategy& strategy, int clientId)
      : app_(app)
      , strategy_(strategy)
      , clientId_(clientId)
  {
  }

 private:
  IBAPI::Application& app_;
  IBAPI::SocketConnector::Strategy& strategy_;
  int clientId_;
  
 public:

  /// @overload EWrapper
  void error(const int id, const int errorCode,
             const IBString errorString) {

    LoggingEWrapper::error(id, errorCode, errorString);
    switch (errorCode) {
      case 326:
        LOG(WARNING) << "Conflicting connection id. Disconnecting.";
        //socket_connector_->disconnect();
        break;
      case 509:
        LOG(WARNING) << "Connection reset. Disconnecting.";
        //socket_connector_->disconnect();
        break;
      case 1100:
        LOG(WARNING) << "Error code = " << errorCode << " disconnecting.";
        //socket_connector_->disconnect();
        break;
      case 502:

      default:
        LOG(WARNING) << "Unhandled Error = " << errorCode << ", do nothing.";
        break;
    }

    //strategy_.onError(errorCode);
  }

  /// @overload EWrapper
  void nextValidId(OrderId orderId) {
    LoggingEWrapper::nextValidId(orderId);
    LOG(INFO) << "Connection confirmed wth next order id = "
              << orderId;
    app_.onLogon(clientId_);
    //IBAPI::NextOrderIdMessage m(orderId);
    //app_.fromAdmin(m, get_connection_id());
  }

  /// @overload EWrapper
  void currentTime(long time) {
    LoggingEWrapper::currentTime(time);

    //IBAPI::HeartBeatMessage m;
    //m.currentTime = time;
    //app_.fromAdmin(m, get_connection_id());
  }


};


} // internal
} // ib

#endif // IB_EVENT_DISPATCHER_H_
