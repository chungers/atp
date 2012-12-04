#ifndef ATP_MAIN_GLOBAL_SETTINGS_H_
#define ATP_MAIN_GLOBAL_SETTINGS_H_

#include <string>

namespace atp {
namespace global {

typedef std::string ConnectorSpec;
typedef std::string OutboundEndpoint;

//// format:  {session_id}={gateway_ip_port}@{reactor_endpoint}
//// comma-delimited

//// firehose - marketdata
const ConnectorSpec FH_CONNECTOR_SPECS =
    "100=127.0.0.1:4001@tcp://127.0.0.1:6666";
const OutboundEndpoint FH_OUTBOUND_ENDPOINTS =
    "0=tcp://127.0.0.1:7777";

//// em - execution / order manager
const ConnectorSpec EM_CONNECTOR_SPECS =
    "100=127.0.0.1:4001@tcp://127.0.0.1:6667";
const OutboundEndpoint EM_OUTBOUND_ENDPOINTS =
    "0=tcp://127.0.0.1:7778";

//// cm - contract manager
const ConnectorSpec CM_CONNECTOR_SPECS =
    "100=127.0.0.1:4001@tcp://127.0.0.1:6668";
const OutboundEndpoint CM_OUTBOUND_ENDPOINTS =
    "0=tcp://127.0.0.1:7779";

//// am - account / porfolio manager
const ConnectorSpec AM_CONNECTOR_SPECS =
    "100=127.0.0.1:4001@tcp://127.0.0.1:6669";
const OutboundEndpoint AM_OUTBOUND_ENDPOINTS =
    "0=tcp://127.0.0.1:7780";




} // global
} // atp


#endif //ATP_MAIN_GLOBAL_SETTINGS_H_
