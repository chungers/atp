#include <sstream>
#include <glog/logging.h>

#include "common.hpp"
#include "log_levels.h"

#include "common/time_utils.hpp"
#include "ib/Application.hpp"
#include "ib/SessionID.hpp"
#include "ib/Exceptions.hpp"
#include "ib/Message.hpp"
#include "ApiImpl.hpp"
//#include "ib/MarketEventDispatcher.hpp"
#include "varz/varz.hpp"

#include "proto/ib.pb.h"


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

template <typename D>
class EventDispatcherEWrapperBase : public LoggingEWrapper, NoCopyAndAssign
{
 public:

  explicit EventDispatcherEWrapperBase(IBAPI::Application& app,
                                       const IBAPI::SessionID& sessionId,
                                       D& dispatcher) :
      app_(app),
      sessionId_(sessionId),
      dispatcher_(dispatcher)
  {
  }

 protected:
  IBAPI::Application& app_;
  const IBAPI::SessionID& sessionId_;
  D& dispatcher_;

  virtual void onNoContractDefinition(int error_code, int req_id)
  {
    LOG(INFO) << "Error " << error_code << " - No security definition.";
  }

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
        onNoContractDefinition(errorCode, id);
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
};


} // internal
} // ib
