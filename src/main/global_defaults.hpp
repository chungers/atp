#ifndef ATP_MAIN_GLOBAL_SETTINGS_H_
#define ATP_MAIN_GLOBAL_SETTINGS_H_

#include <string>
#include <ostream>

#include <boost/lexical_cast.hpp>

using namespace std;

namespace atp {
namespace global {

typedef unsigned int Port;
typedef unsigned int SessionId;
typedef unsigned int ChannelId;
typedef string Host;
typedef string ZmqEndPoint;
typedef string ConnectorSpec;
typedef string OutboundEndPoint;

//// format:  {session_id}={gateway_ip_port}@{reactor_endpoint}
//// comma-delimited

string build_connector_spec(const SessionId& id,
                            const string& ib_gateway_host,
                            const Port& ib_gateway_port,
                            const ZmqEndPoint& controller_endpoint)
{
  ostringstream oss;
  oss << id << '=' << ib_gateway_host << ':' << ib_gateway_port
      << '@' << controller_endpoint;
  return oss.str();
}

string build_outbound_spec(const ZmqEndPoint& outbound_endpoint,
                           const ChannelId& channel = 0)
{
  ostringstream oss;
  oss << channel << '=' << outbound_endpoint;
  return oss.str();
}


//// firehose - marketdata
const Host FH_IBG_HOST = "127.0.0.1";
const Port FH_IBG_PORT = 4001;
const ZmqEndPoint FH_CONTROLLER_ENDPOINT = "tcp://127.0.0.1:5500";
const ZmqEndPoint FH_OUTBOUND_ENDPOINT = "tcp://127.0.0.1:7700";
const SessionId FH_SESSION_ID = 100;
const Port FH_VARZ_PORT = 38000;
const bool FH_OUTBOUND_PUBLISH = true;

const ConnectorSpec FH_CONNECTOR_SPECS = build_connector_spec(
    FH_SESSION_ID,
    FH_IBG_HOST,
    FH_IBG_PORT,
    FH_CONTROLLER_ENDPOINT);
const OutboundEndPoint FH_OUTBOUND_ENDPOINTS = build_outbound_spec(
    FH_OUTBOUND_ENDPOINT);


//// em - execution manager
const Host EM_IBG_HOST = "127.0.0.1";
const Port EM_IBG_PORT = 4001;
const ZmqEndPoint EM_CONTROLLER_ENDPOINT = "tcp://127.0.0.1:5501";
const ZmqEndPoint EM_OUTBOUND_ENDPOINT = "tcp://127.0.0.1:7701";
const SessionId EM_SESSION_ID = 101;
const Port EM_VARZ_PORT = 38001;
const bool EM_OUTBOUND_PUBLISH = true;

const ConnectorSpec EM_CONNECTOR_SPECS = build_connector_spec(
    EM_SESSION_ID,
    EM_IBG_HOST,
    EM_IBG_PORT,
    EM_CONTROLLER_ENDPOINT);
const OutboundEndPoint EM_OUTBOUND_ENDPOINTS = build_outbound_spec(
    EM_OUTBOUND_ENDPOINT);

//// cm - contract manager
const Host CM_IBG_HOST = "127.0.0.1";
const Port CM_IBG_PORT = 4001;
const ZmqEndPoint CM_CONTROLLER_ENDPOINT = "tcp://127.0.0.1:5502";
const ZmqEndPoint CM_OUTBOUND_ENDPOINT = "tcp://127.0.0.1:7702";
const SessionId CM_SESSION_ID = 102;
const Port CM_VARZ_PORT = 38002;
const bool CM_OUTBOUND_PUBLISH = true;

const ConnectorSpec CM_CONNECTOR_SPECS = build_connector_spec(
    CM_SESSION_ID,
    CM_IBG_HOST,
    CM_IBG_PORT,
    CM_CONTROLLER_ENDPOINT);
const OutboundEndPoint CM_OUTBOUND_ENDPOINTS = build_outbound_spec(
    CM_OUTBOUND_ENDPOINT);




} // global
} // atp


#endif //ATP_MAIN_GLOBAL_SETTINGS_H_
