#ifndef ATP_MAIN_GLOBAL_SETTINGS_H_
#define ATP_MAIN_GLOBAL_SETTINGS_H_

#include <string>
#include <ostream>

#include <boost/lexical_cast.hpp>

#include "zmq/ZmqUtils.hpp"



using namespace std;

namespace atp {
namespace global {

typedef unsigned int Port;
typedef unsigned int SessionId;
typedef unsigned int ChannelId;
typedef string Host;
typedef string ZmqEndPoint;
typedef string CouchDbEndPoint;
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
const Host FH_HOST = "127.0.0.1"; // for client socket connect
const Host FH_BIND_HOST = "*";  // for server socket bind
const Port FH_CONTROLLER_PORT = 5500;
const Port FH_OUTBOUND_PORT = 7700;
const ZmqEndPoint FH_CONTROLLER_ENDPOINT = atp::zmq::EndPoint::tcp(
    FH_CONTROLLER_PORT, FH_BIND_HOST);
const ZmqEndPoint FH_OUTBOUND_ENDPOINT = atp::zmq::EndPoint::tcp(
    FH_OUTBOUND_PORT, FH_BIND_HOST);
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

//// lp - log publisher
const Host LP_HOST = FH_HOST; // for client socket connect
const Port LP_OUTBOUND_PORT = FH_OUTBOUND_PORT;

//// em - execution manager
const Host EM_IBG_HOST = "127.0.0.1";
const Port EM_IBG_PORT = 4001;
const Host EM_HOST = "127.0.0.1"; // for client socket connect
const Host EM_BIND_HOST = "*"; // for server socket bind
const Port EM_CONTROLLER_PORT = 5501;
const Port EM_OUTBOUND_PORT = 7701;
const ZmqEndPoint EM_CONTROLLER_ENDPOINT = atp::zmq::EndPoint::tcp(
    EM_CONTROLLER_PORT, EM_BIND_HOST);
const ZmqEndPoint EM_OUTBOUND_ENDPOINT = atp::zmq::EndPoint::tcp(
    EM_OUTBOUND_PORT, EM_BIND_HOST);
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
const Host CM_HOST = "127.0.0.1"; // for client socket connect
const Host CM_BIND_HOST = "*";  // for server socket bind
const Port CM_CONTROLLER_PORT = 5502;
const Port CM_OUTBOUND_PORT = 7702;
const ZmqEndPoint CM_CONTROLLER_ENDPOINT = atp::zmq::EndPoint::tcp(
    CM_CONTROLLER_PORT, CM_BIND_HOST);
const ZmqEndPoint CM_OUTBOUND_ENDPOINT = atp::zmq::EndPoint::tcp(
    CM_OUTBOUND_PORT, CM_BIND_HOST);
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

//// hz - historian server
const ZmqEndPoint HZ_QUERY_ENDPOINT = "tcp://127.0.0.1:5503";
const ZmqEndPoint HZ_ADMIN_ENDPOINT = "tcp://127.0.0.1:5504";
const ZmqEndPoint HZ_EVENT_ENDPOINT = "tcp://127.0.0.1:7703"; // agent events
const ZmqEndPoint HZ_SUBSCRIBER_ENDPOINT = FH_OUTBOUND_ENDPOINT;
const Port HZ_VARZ_PORT = 38003;

//// hzc - historian client
const ZmqEndPoint HZC_RESULT_STREAM_ENDPOINT = "tcp://127.0.0.1:6600";

const CouchDbEndPoint CONTRACT_DB_ENDPOINT = "http://127.0.0.1:5984";

} // global
} // atp


#endif //ATP_MAIN_GLOBAL_SETTINGS_H_
