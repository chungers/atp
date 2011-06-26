#ifndef IB_DISPATCHER_H_
#define IB_DISPATCHER_H_

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "ib/Application.hpp"
#include "ib/Initiator.hpp"
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

  EventDispatcher(ib::Application& app)
      : app_(app)
  {
  }

 private:
  Application& app_;

 public:

  /** @override EWrapper */
  void error(const int id, const int errorCode,
             const IBString errorString) {

    LoggingEWrapper::error(id, errorCode, errorString);
    initiator_.onConnect();
  }

  /** @override EWrapper */
  void nextValidId(OrderId orderId) {
    LoggingEWrapper::nextValidId(orderId);
    LOG(INFO) << "Connection confirmed wth next order id = "
              << orderId;
    initiator_.onConnect();

    NextOrderIdMessage m;
    m.nextOrderId = orderId;
    app_.fromAdmin(m, get_connection_id());
  }

  /** @implements EWrapper */
  void currentTime(long time) {
    LoggingEWrapper::currentTime(time);

    initiator_.onHeartBeat(time);

    HeartBeatMessage m;
    m.currentTime = time;
    app_.fromAdmin(m, get_connection_id());
  }


};


} // internal
} // ib

#endif // IB_DISPATCHER_H_
