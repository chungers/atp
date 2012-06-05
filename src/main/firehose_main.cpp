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

#include "constants.h"
#include "ib/ApiEventDispatcher.hpp"
#include "ib/ApplicationBase.hpp"
#include "ib/SocketInitiator.hpp"
#include "ib/MarketEventDispatcher.hpp"
#include "varz/varz.hpp"
#include "varz/VarzServer.hpp"




static IBAPI::SocketInitiator* INITIATOR_INSTANCE;
static atp::varz::VarzServer* VARZ_INSTANCE;


/// format:  {session_id}={gateway_ip_port}@{reactor_endpoint}
const std::string CONNECTOR_SPECS =
    "100=127.0.0.1:4001@tcp://127.0.0.1:6666";

const std::string OUTBOUND_ENDPOINTS = "0=tcp://127.0.0.1:7777";

DEFINE_string(connectors, CONNECTOR_SPECS,
              "Comma-delimited list of gateway ip/port @ control endpoints.");
DEFINE_string(outbound, OUTBOUND_ENDPOINTS,
              "Comma-delimited list of channel and outbound endpoints");
DEFINE_bool(publish, false,
            "True to publish at outbound endpoints, false to push to them");
DEFINE_int32(varz, 18000, "The port varz server runs on.");

DEFINE_VARZ_bool(fh_as_publisher, false, "if instance is also a publisher.");
DEFINE_VARZ_string(fh_connector_specs, "", "Connector specs");
DEFINE_VARZ_string(fh_outbound_endpoints, "", "Outbound endpoints");

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
const set<string> FIREHOSE_VALID_MESSAGES_ =
               boost::assign::list_of
               ("IBAPI.FEED.RequestMarketData")
               ("IBAPI.FEED.CancelMarketData")
               ("IBAPI.FEED.RequestMarketDepth")
               ("IBAPI.FEED.CancelMarketDepth")
               ;

using std::map;
using std::vector;
using std::string;
using std::stringstream;
using std::istringstream;
using IBAPI::SessionID;
using IBAPI::SessionSetting;
using IBAPI::SocketInitiator;
using IBAPI::ApiEventDispatcher;


class Firehose : public IBAPI::ApplicationBase
{
 public:

  Firehose() {}
  ~Firehose() {}

  virtual bool IsMessageSupported(const std::string& key)
  {
    return FIREHOSE_VALID_MESSAGES_.find(key) != FIREHOSE_VALID_MESSAGES_.end();
  }

  virtual ApiEventDispatcher* GetApiEventDispatcher(const SessionID& sessionId)
  {
    return new ib::internal::MarketEventDispatcher(*this, sessionId);
  }

  void onLogon(const SessionID& sessionId)
  {
    LOG(INFO) << "Session " << sessionId << " logged on.";
  }

  void onLogout(const SessionID& sessionId)
  {
    LOG(INFO) << "Session " << sessionId << " logged off.";
  }

};


////////////////////////////////////////////////////////
//
// MAIN
//
// Firehose - Market data gateway
//
int main(int argc, char** argv)
{
  google::SetUsageMessage("Firehose connecting message queue and IB gateways.");
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  atp::varz::Varz::initialize();

  LOG(INFO) << "Firehose - IB API version: " << IB_API_VERSION;

  // Signal handler: Ctrl-C
  signal(SIGINT, OnTerminate);
  // Signal handler: kill -TERM pid
  void (*terminate)(int);
  terminate = signal(SIGTERM, OnTerminate);
  if (terminate == SIG_IGN) {
    LOG(INFO) << "RESETTING SIGNAL SIGTERM";
    signal(SIGTERM, SIG_IGN);
  }

  VARZ_fh_as_publisher = FLAGS_publish;
  VARZ_fh_connector_specs = FLAGS_connectors;
  VARZ_fh_outbound_endpoints = FLAGS_outbound;

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

    Firehose firehose;
    SocketInitiator initiator(firehose, settings);

    INITIATOR_INSTANCE = &initiator;

    if (SocketInitiator::Configure(initiator, outboundMap, FLAGS_publish)) {

      LOG(INFO) << "Start connections";
      initiator.start();

      // Now waiting for shutdown
      initiator.block();

      LOG(INFO) << "Firehose terminated.";
      return 0;
    }
  }

  LOG(ERROR) << "Invalid flags: " << FLAGS_connectors
             << ", " << FLAGS_outbound;
  return 1;
}



