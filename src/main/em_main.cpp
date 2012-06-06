#include <signal.h>
#include <sstream>
#include <map>
#include <set>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/assign.hpp>
#include <boost/date_time/gregorian/greg_month.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <boost/format.hpp>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "varz/varz.hpp"
#include "varz/VarzServer.hpp"

#include "constants.h"
#include "ib/ApplicationBase.hpp"
#include "ib/SocketInitiator.hpp"
#include "ib/OrderEventDispatcher.hpp"


static IBAPI::SocketInitiator* INITIATOR_INSTANCE;
static atp::varz::VarzServer* VARZ_INSTANCE;


/// format:  {session_id}={gateway_ip_port}@{reactor_endpoint}
const std::string CONNECTOR_SPECS =
    "200=127.0.0.1:4001@tcp://127.0.0.1:6667";

const std::string OUTBOUND_ENDPOINTS = "0=tcp://127.0.0.1:8888";

DEFINE_string(connectors, CONNECTOR_SPECS,
              "Comma-delimited list of gateway ip/port @ control endpoints.");
DEFINE_string(outbound, OUTBOUND_ENDPOINTS,
              "Comma-delimited list of channel and outbound endpoints");
DEFINE_bool(publish, false,
            "True to publish at outbound endpoints, false to push to them");
DEFINE_int32(varz, 28000, "The port varz server runs on.");

DEFINE_VARZ_bool(em_as_publisher, false, "if instance is also a publisher.");
DEFINE_VARZ_string(em_connector_specs, "", "Connector specs");
DEFINE_VARZ_string(em_outbound_endpoints, "", "Outbound endpoints");

DEFINE_VARZ_string(ib_api_version, IB_API_VERSION, "IB api version");


void OnTerminate(int param)
{
  LOG(INFO) << "===================== SHUTTING DOWN =======================";
  if (INITIATOR_INSTANCE) {
    INITIATOR_INSTANCE->stop();
    LOG(INFO) << "Stopped initiator.";
  }
  if (VARZ_INSTANCE) {
    VARZ_INSTANCE->stop();
    LOG(INFO) << "Stopped varz.";
  }
  LOG(INFO) << "Bye.";
  exit(1);
}

// Firehose only supports messages related to market data.
const set<string> EM_VALID_MESSAGES_ =
               boost::assign::list_of
               ("IBAPI.EXEC.CancelOrder")
               ("IBAPI.EXEC.MarketOrder")
               ("IBAPI.EXEC.LimitOrder")
               ("IBAPI.EXEC.StopLimitOrder")
               ;

using std::map;
using std::vector;
using std::string;
using std::stringstream;
using std::istringstream;
using IBAPI::ApiEventDispatcher;
using IBAPI::SessionID;
using IBAPI::SessionSetting;
using IBAPI::SocketInitiator;


class ExecutionManager : public IBAPI::ApplicationBase
{
 public:

  ExecutionManager() {}
  ~ExecutionManager() {}


  virtual bool IsMessageSupported(const std::string& key)
  {
    return EM_VALID_MESSAGES_.find(key) != EM_VALID_MESSAGES_.end();
  }

  virtual ApiEventDispatcher* GetApiEventDispatcher(const SessionID& sessionId)
  {
    return new ib::internal::OrderEventDispatcher(*this, sessionId);
  }

  void onLogon(const IBAPI::SessionID& sessionId)
  {
    LOG(INFO) << "Session " << sessionId << " logged on.";
  }

  void onLogout(const IBAPI::SessionID& sessionId)
  {
    LOG(INFO) << "Session " << sessionId << " logged off.";
  }

};


////////////////////////////////////////////////////////
//
// MAIN
//
// EM - Execution / Order Manager
//
int main(int argc, char** argv)
{
  google::SetUsageMessage("EM - Execution Manager");
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  atp::varz::Varz::initialize();

  LOG(INFO) << "EM - IB API version: " << IB_API_VERSION;

  // Signal handler: Ctrl-C
  signal(SIGINT, OnTerminate);
  // Signal handler: kill -TERM pid
  void (*terminate)(int);
  terminate = signal(SIGTERM, OnTerminate);
  if (terminate == SIG_IGN) {
    LOG(INFO) << "RESETTING SIGNAL SIGTERM";
    signal(SIGTERM, SIG_IGN);
  }

  VARZ_em_as_publisher = FLAGS_publish;
  VARZ_em_connector_specs = FLAGS_connectors;
  VARZ_em_outbound_endpoints = FLAGS_outbound;

  LOG(INFO) << "Starting varz at " << FLAGS_varz;
  atp::varz::VarzServer varz(FLAGS_varz, 2);
  VARZ_INSTANCE = &varz;
  varz.start();

  // SessionSettings for the initiator
  SocketInitiator::SessionSettings settings;

  // Outbound publisher endpoints for different channels
  map<int, string> outboundMap;

  if (SocketInitiator::ParseSessionSettingsFromFlag(
          FLAGS_connectors, settings) &&
      SocketInitiator::ParseOutboundChannelMapFromFlag(
          FLAGS_outbound, outboundMap)) {

    LOG(INFO) << "Starting initiator.";

    ExecutionManager em;
    SocketInitiator initiator(em, settings);

    INITIATOR_INSTANCE = &initiator;

    if (SocketInitiator::Configure(initiator, outboundMap, FLAGS_publish)) {

      LOG(INFO) << "Start connections";
      initiator.start();

      // Now waiting for shutdown
      initiator.block();

      LOG(INFO) << "ExecutionManager terminated.";
      return 0;
    }
  }

  LOG(ERROR) << "Invalid flags: " << FLAGS_connectors
             << ", " << FLAGS_outbound;
  return 1;
}



