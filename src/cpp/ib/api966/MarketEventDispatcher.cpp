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
#include "ib/MarketEventDispatcher.hpp"

#include "EventDispatcherEWrapperBase.hpp"


namespace ib {
namespace internal {


class MarketEventEWrapper :
      public EventDispatcherEWrapperBase<MarketEventDispatcher>
{
 public:

  explicit MarketEventEWrapper(IBAPI::Application& app,
                               const IBAPI::SessionID& sessionId,
                               MarketEventDispatcher& dispatcher) :
      EventDispatcherEWrapperBase<MarketEventDispatcher>(
          app, sessionId, dispatcher)
  {
  }

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

  /// @overload EWrapper
  void tickPrice(TickerId tickerId, TickType field, double price,
                 int canAutoExecute)
  {
    LoggingEWrapper::tickPrice(tickerId, field, price, canAutoExecute);
    dispatcher_.publish<double>(tickerId, field, price, *this);
  }

  /// @overload EWrapper
  void tickSize(TickerId tickerId, TickType field, int size)
  {
    LoggingEWrapper::tickSize(tickerId, field, size);
    dispatcher_.publish<int>(tickerId, field, size, *this);
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
    dispatcher_.publish<double>(tickerId, tickType, value, *this);
  }

  /// @overload EWrapper
   void tickString(TickerId tickerId, TickType tickType,
                   const IBString& value)
  {
    LoggingEWrapper::tickString(tickerId, tickType, value);
    dispatcher_.publish<std::string>(tickerId, tickType, value, *this);
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
    dispatcher_.publishDepth(id, side, position, operation, price, size, *this);
  }

  /// @overload EWrapper
  void updateMktDepthL2(TickerId id, int position,
                        IBString marketMaker, int operation,
                        int side, double price, int size)
  {
    LoggingEWrapper::updateMktDepthL2(id, position, marketMaker, operation,
                                      side, price, size);
    dispatcher_.publishDepth(id, side, position, operation, price, size, *this,
                             marketMaker);
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


EWrapper* MarketEventDispatcher::GetEWrapper()
{
  return new MarketEventEWrapper(Application(), SessionId(), *this);
}


} // internal
} // ib
