#include <sstream>
#include <glog/logging.h>

#include "common.hpp"
#include "ib/Application.hpp"
#include "ib/SessionID.hpp"
#include "ib/Exceptions.hpp"
#include "ib/Message.hpp"
#include "ApiImpl.hpp"
#include "ib/MarketEventDispatcher.hpp"
#include "varz/varz.hpp"


DEFINE_VARZ_int64(ib_api_error_no_security_definition, 0, "");
DEFINE_VARZ_int64(ib_api_error_unhandled, 0, "");
DEFINE_VARZ_int64(ib_api_error_connection_reset, 0, "");
DEFINE_VARZ_int64(ib_api_error_gateway_disconnect, 0, "");
DEFINE_VARZ_int64(ib_api_error_marketdata_farm_connection_broken, 0, "");
DEFINE_VARZ_int64(ib_api_error_marketdata_farm_connection_ok, 0, "");
DEFINE_VARZ_int64(ib_api_error_gateway_server_connection_broken, 0, "");
DEFINE_VARZ_int64(ib_api_error_cannot_connect, 0, "");


namespace ib {
namespace internal {


class MarketEventEWrapper : public LoggingEWrapper, NoCopyAndAssign
{
 public:

  explicit MarketEventEWrapper(IBAPI::Application& app,
                               const IBAPI::SessionID& sessionId,
                               MarketEventDispatcher& dispatcher) :
      app_(app),
      sessionId_(sessionId),
      dispatcher_(dispatcher)
  {
  }

 private:
  IBAPI::Application& app_;
  const IBAPI::SessionID& sessionId_;
  MarketEventDispatcher& dispatcher_;

 public:

  /// @overload EWrapper
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
        VARZ_ib_api_error_no_security_definition++;
        msg << "No security definition has been found for the request, id="
            << id;
        break;
      case 326:
        terminate = true;
        msg << "Conflicting connection id. Disconnecting.";
        break;
      case 502:
        terminate = false; // Set to false to avoid hanging connection on linux
        VARZ_ib_api_error_cannot_connect++;
        msg << "Couldn't connect to TWS.  "
            << "Confirm that \"Enable ActiveX and Socket Clients\" is enabled on the TWS "
            << "\"Configure->API\" menu.";
        break;
      case 509:
        terminate = false; // Set to false to avoid hanging connection on linux
        VARZ_ib_api_error_connection_reset++;
        msg << "Connection reset. Disconnecting.";
        break;
      case 1100:
        LOG(ERROR) << "Error 1100 -- not disconnecting. Expect IBGateway to reconnect.";
        terminate = false;
        VARZ_ib_api_error_gateway_disconnect++;
        msg << "No disconnect.";
        break;
      case 2103:
        terminate = false;
        VARZ_ib_api_error_marketdata_farm_connection_broken++;
        msg << "Market data farm connection is broken.";
        break;
      case 2104:
        terminate = false;
        VARZ_ib_api_error_marketdata_farm_connection_ok++;
        msg << "Market data farm connection is OK";
        break;
      case 2110:
        terminate = false;
        VARZ_ib_api_error_gateway_server_connection_broken++;
        msg << "Connectivity between TWS and server is broken. It will be restored automatically.";
        break;

      default:
        terminate = false;
        VARZ_ib_api_error_unhandled++;
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
