
#include <sstream>
#include <map>
#include <vector>

#include <boost/algorithm/string.hpp>
#include "boost/assign.hpp"
#include <boost/date_time/gregorian/greg_month.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <boost/format.hpp>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "ib/SocketInitiator.hpp"


static IBAPI::SocketInitiator* INITIATOR_INSTANCE;

/// format:  {session_id}={gateway_ip_port}@{reactor_endpoint}
const std::string CONNECTOR_SPECS =
    "100=127.0.0.1:4001@tcp://127.0.0.1:5551";

const std::string OUTBOUND_ENDPOINTS =
    "0=tcp://127.0.0.1:6666,1=tcp://127.0.0.1:6667";

DEFINE_string(connectors, CONNECTOR_SPECS,
              "Comma-delimited list of gateway ip/port @ control endpoints.");
DEFINE_string(outbound, OUTBOUND_ENDPOINTS,
              "Comma-delimited list of channel and outbound endpoints");
DEFINE_bool(publish, true,
            "True to publish at outbound endpoints, false to push to them");


void OnTerminate(int param)
{
  LOG(INFO) << "===================== SHUTTING DOWN =======================";
  if (INITIATOR_INSTANCE) {
    INITIATOR_INSTANCE->stop();
  }

  LOG(INFO) << "Bye.";
  exit(1);
}

class Firehose : public IBAPI::ApplicationBase
{
 public:

  Firehose() {}
  ~Firehose() {}

  void onLogon(const IBAPI::SessionID& sessionId)
  {
    LOG(INFO) << "Session " << sessionId << " logged on.";
  }

  void onLogout(const IBAPI::SessionID& sessionId)
  {
    LOG(INFO) << "Session " << sessionId << " logged off.";
  }
};

using std::map;
using std::vector;
using std::string;
using std::stringstream;
using std::istringstream;
using IBAPI::SessionSetting;
using IBAPI::SocketInitiator;

////////////////////////////////////////////////////////
//
// MAIN
//
int main(int argc, char** argv)
{
  google::SetUsageMessage("Firehose connecting message queue and IB gateways.");
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  // Signal handler: Ctrl-C
  signal(SIGINT, OnTerminate);
  // Signal handler: kill -TERM pid
  void (*terminate)(int);
  terminate = signal(SIGTERM, OnTerminate);
  if (terminate == SIG_IGN) {
    LOG(INFO) << "RESETTING SIGNAL SIGTERM";
    signal(SIGTERM, SIG_IGN);
  }

  // Get the connector specs
  vector<string> connectorSpecs;
  boost::split(connectorSpecs, FLAGS_connectors, boost::is_any_of(","));

  SocketInitiator::SessionSettings settings;
  for (vector<string>::iterator spec = connectorSpecs.begin();
       spec != connectorSpecs.end(); ++spec) {

    /// format:  {session_id}={gateway_ip_port}@{reactor_endpoint}
    stringstream iss(*spec);

    string sessionIdStr;
    std::getline(iss, sessionIdStr, '=');
    string gateway;
    std::getline(iss, gateway, '@');
    string reactor;
    iss >> reactor;

    istringstream session_parse(sessionIdStr);
    unsigned int sessionId = 0;
    session_parse >> sessionId;

    // Need to split the gateway
    vector<string> gatewayParts;
    boost::split(gatewayParts, gateway, boost::is_any_of(":"));
    istringstream port_parse(gatewayParts[1]);
    int port = 0;
    port_parse >> port;

    string host = gatewayParts[0];

    LOG(INFO) << "session = " << sessionId << ", "
              << "gateway = " << host << ":" << port << ", "
              << "reactor = " << reactor;

    SessionSetting setting(sessionId, host, port, reactor);
    settings.push_back(setting);
  }

  // Outbound publisher endpoints for different channels
  map<int, string> outboundMap;

  vector<string> outboundSpecs;
  boost::split(outboundSpecs, FLAGS_outbound, boost::is_any_of(","));
  for (vector<string>::iterator spec = outboundSpecs.begin();
       spec != outboundSpecs.end(); ++spec) {

    /// format:  {channel_id}={push_endpoint}
    stringstream iss(*spec);

    string channelIdStr;
    std::getline(iss, channelIdStr, '=');
    string endpoint;
    iss >> endpoint;

    istringstream channel_parse(channelIdStr);
    int channel = 0;
    channel_parse >> channel;

    LOG(INFO) << "channel = " << channel << ", "
              << "endpoint = " << endpoint;

    outboundMap[channel] = endpoint;
  }

  LOG(INFO) << "Starting initiator.";
  Firehose firehose;
  SocketInitiator initiator(firehose, settings);

  INITIATOR_INSTANCE = &initiator;

  map<int, string>::iterator outboundEndpoint = outboundMap.begin();
  for (; outboundEndpoint != outboundMap.end(); ++outboundEndpoint) {
    int channel = outboundEndpoint->first;
    string endpoint = outboundEndpoint->second;

    if (FLAGS_publish) {
      LOG(INFO) << "Channel " << channel << ", PUBLISH to " << endpoint;
      initiator.publish(channel, endpoint);
    } else {
      LOG(INFO) << "Channel " << channel << ", PUSH to " << endpoint;
      initiator.push(channel, endpoint);
    }
  }

  LOG(INFO) << "Start connections";
  initiator.start();

  initiator.block();
  return 0;
}



