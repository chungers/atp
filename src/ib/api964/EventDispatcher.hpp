#ifndef IB_EVENT_DISPATCHER_H_
#define IB_EVENT_DISPATCHER_H_

#include <sstream>
#include <glog/logging.h>

#include "ib/Application.hpp"
#include "ib/Exceptions.hpp"
#include "ib/Message.hpp"
#include "ApiImpl.hpp"
#include "ib/EventDispatcherBase.hpp"


namespace ib {
namespace internal {


/**
 * Basic design follows SocketConnector / SocketInitiator in QuickFIX.
 *
 * See https://github.com/lab616/third_party/blob/master/quickfix-1.13.3/src/C++/SocketConnector.h
 */
class EventDispatcher : public EventDispatcherBase, public LoggingEWrapper
{
 public:

  EventDispatcher(IBAPI::Application& app,
                  EWrapperEventCollector& eventCollector,
                  int clientId) :
      EventDispatcherBase(eventCollector),
      app_(app),
      clientId_(clientId)
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

    switch (errorCode) {

      // The following code will cause exception to be thrown, which
      // will force the event loop to terminate.
      case 200:
        terminate = false;
        msg << "No security definition has been found for the request, id="
            << id;
        break;
      case 326:
        terminate = true;
        msg << "Conflicting connection id. Disconnecting.";
        break;
      case 502:
        terminate = false; // Set to false to avoid hanging connection on linux
        msg << "Couldn't connect to TWS.  "
            << "Confirm that \"Enable ActiveX and Socket Clients\" is enabled on the TWS "
            << "\"Configure->API\" menu.";
        break;
      case 509:
        terminate = false; // Set to false to avoid hanging connection on linux
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
  }

  /// @overload EWrapper
  void currentTime(long time)
  {
    LoggingEWrapper::currentTime(time);
    // TODO send some application admin message??
  }

  /// @overload EWrapper
  void tickPrice(TickerId tickerId, TickType field, double price,
                 int canAutoExecute)
  {
    LoggingEWrapper::tickPrice(tickerId, field, price, canAutoExecute);
    publish<double>(tickerId, field, price, *this);
  }

  /// @overload EWrapper
  void tickSize(TickerId tickerId, TickType field, int size)
  {
    LoggingEWrapper::tickSize(tickerId, field, size);
    publish<int>(tickerId, field, size, *this);
  }

  /// @overload EWrapper
  void tickOptionComputation(TickerId tickerId, TickType tickType,
                             double impliedVol, double delta,
                             double optPrice, double pvDividend,
                             double gamma, double vega, double theta,
                             double undPrice)
  {
    LoggingEWrapper::tickOptionComputation(tickerId, tickType,
                                           impliedVol, delta,
                                           optPrice, pvDividend,
                                           gamma, vega, theta,
                                           undPrice);
  }

  /// @overload EWrapper
   void tickGeneric(TickerId tickerId, TickType tickType, double value)
  {
    LoggingEWrapper::tickGeneric(tickerId, tickType, value);
    publish<double>(tickerId, tickType, value, *this);
  }

  /// @overload EWrapper
   void tickString(TickerId tickerId, TickType tickType,
                   const IBString& value)
  {
    LoggingEWrapper::tickString(tickerId, tickType, value);
    publish<std::string>(tickerId, tickType, value, *this);
  }

  /// @overload EWrapper
  void tickSnapshotEnd(int reqId)
  {
    LoggingEWrapper::tickSnapshotEnd(reqId);
  }

  /// @overload EWrapper
  void updateMktDepth(TickerId id, int position, int operation, int side,
                      double price, int size)
  {
    LoggingEWrapper::updateMktDepth(id, position, operation, side,
                                    price, size);
  }

  /// @overload EWrapper
  void updateMktDepthL2(TickerId id, int position,
                        IBString marketMaker, int operation,
                        int side, double price, int size)
  {
    LoggingEWrapper::updateMktDepthL2(id, position, marketMaker, operation,
                                      side, price, size);
  }

  /// @overload EWrapper
  void realtimeBar(TickerId reqId, long time,
                   double open, double high, double low, double close,
                   long volume, double wap, int count)
  {
    LoggingEWrapper::realtimeBar(reqId, time, open, high, low, close,
                                 volume, wap, count);
  }

  /// @overload EWrapper
    void historicalData(TickerId reqId, const IBString& date,
                        double open, double high,
                        double low, double close, int volume,
                        int barCount, double WAP, int hasGaps)
  {
    LoggingEWrapper::historicalData(reqId, date, open, high, low, close,
                                    volume, barCount, WAP, hasGaps);
  }

};


} // internal
} // ib

#endif // IB_EVENT_DISPATCHER_H_
