
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


/// format:  {session_id}={gateway_ip_port}@{reactor_endpoint}
const std::string CONNECTOR_SPECS =
    "100=127.0.0.1:4001@tcp://127.0.0.1:5555";

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
  LOG(INFO) << "Bye.";
  exit(1);
}


using std::vector;
using std::string;
using std::stringstream;
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

    string sessionId;
    std::getline(iss, sessionId, '=');
    string gateway;
    std::getline(iss, gateway, '@');
    string reactor;
    iss >> reactor;

    LOG(INFO) << "session = " << sessionId << ", "
              << "gateway = " << gateway << ", "
              << "reactor = " << reactor;
  }

  return 0;
}



