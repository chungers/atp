#ifndef IB_EVENT_DISPATCHER_H_
#define IB_EVENT_DISPATCHER_H_

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "ib/Application.hpp"
#include "ib/Message.hpp"
#include "ib/SocketConnector.hpp"
#include "ib/logging_impl.hpp"

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

  EventDispatcher(IBAPI::Application& app, IBAPI::SocketConnector::Strategy& strategy)
      : app_(app)
      , strategy_(strategy)
  {
  }

 private:
  IBAPI::Application& app_;
  IBAPI::SocketConnector::Strategy& strategy_;

 public:

  /** @override EWrapper */
  void error(const int id, const int errorCode,
             const IBString errorString) {

    LoggingEWrapper::error(id, errorCode, errorString);
    //strategy_.onError(errorCode);
  }

  /** @override EWrapper */
  void nextValidId(OrderId orderId) {
    LoggingEWrapper::nextValidId(orderId);
    LOG(INFO) << "Connection confirmed wth next order id = "
              << orderId;
    //strategy_.onConnect();

    IBAPI::NextOrderIdMessage m(orderId);
    app_.fromAdmin(m, get_connection_id());
  }

  /** @implements EWrapper */
  void currentTime(long time) {
    LoggingEWrapper::currentTime(time);

    IBAPI::HeartBeatMessage m;
    m.currentTime = time;
    app_.fromAdmin(m, get_connection_id());
  }


};


} // internal
} // ib

#endif // IB_EVENT_DISPATCHER_H_
