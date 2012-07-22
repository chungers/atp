#ifndef ATP_LOG_LEVELS_H_
#define ATP_LOG_LEVELS_H_

#include <glog/logging.h>

// Verbose level.  Use flag --v=N where N >= VLOG_LEVEL_* to see.

#define IBAPI_APPLICATION_LOGGER VLOG(1)

#define ECLIENT_LOGGER LOG(INFO)
#define EWRAPPER_LOGGER LOG(INFO)

#define ASIO_ECLIENT_SOCKET_LOGGER VLOG(10)
#define ASIO_ECLIENT_SOCKET_DEBUG VLOG(30)

#define IBAPI_SOCKET_INITIATOR_LOGGER VLOG(5)
#define IBAPI_SOCKET_CONNECTOR_LOGGER VLOG(50)
#define IBAPI_SOCKET_CONNECTOR_ERROR LOG(INFO)
#define IBAPI_SOCKET_CONNECTOR_WARNING LOG(WARNING)

#define IBAPI_SOCKET_CONNECTOR_STRATEGY_LOGGER VLOG(10)
#define IBAPI_ABSTRACT_SOCKET_CONNECTOR_LOGGER VLOG(20)

#define TICKERMAP_LOGGER VLOG(50)
#define TICKERMAP_ERROR LOG(ERROR)

#define MANAGED_AGENT_LOGGER LOG(INFO)
#define MANAGED_AGENT_ERROR LOG(ERROR)

// Varz
#define VARZ_LOGGER VLOG(20)
#define VARZ_DEBUG VLOG(50)
#define VARZ_ERROR LOG(ERROR)

// Zmq
#define ZMQ_MEM_FREE_DEBUG VLOG(100)
#define ZMQ_SOCKET_DEBUG VLOG(100)
#define ZMQ_REACTOR_LOGGER VLOG(20)
#define ZMQ_PUBLISHER_LOGGER VLOG(20)

#define ZMQ_SUBSCRIBER_LOGGER VLOG(20)
#define ZMQ_SUBSCRIBER_ERROR LOG(ERROR)

// ApiMessageBase
#define API_MESSAGE_BASE_LOGGER VLOG(40)
#define API_MESSAGE_STRATEGY_LOGGER VLOG(40)

// ApiMessages
#define API_MESSAGES_LOGGER VLOG(40)
#define API_MESSAGES_ERROR LOG(INFO)

// ProtoBufferMessage
#define ZMQ_PROTO_MESSAGE_LOGGER VLOG(40)
#define ZMQ_PROTO_MESSAGE_ERROR LOG(INFO)

// LogReader
#define LOG_READER_LOGGER VLOG(10)
#define LOG_READER_DEBUG VLOG(20)

// Historian
#define HISTORIAN_LOGGER VLOG(10)
#define HISTORIAN_DEBUG VLOG(20)

// Historian reactor
#define HISTORIAN_REACTOR_LOGGER VLOG(10)
#define HISTORIAN_REACTOR_ERROR LOG(ERROR)
#define HISTORIAN_REACTOR_DEBUG VLOG(20)

// firehose
#define FIREHOSE_LOGGER VLOG(10)
#define FIREHOSE_DEBUG VLOG(20)

// MarketDataSubscriber
#define MARKET_DATA_SUBSCRIBER_LOGGER LOG(INFO)
#define MARKET_DATA_SUBSCRIBER_DEBUG VLOG(20)

// OrderManager
#define ORDER_MANAGER_LOGGER LOG(INFO)
#define ORDER_MANAGER_DEBUG VLOG(20)
#define ORDER_MANAGER_ERROR LOG(ERROR)

#endif //ATP_LOG_LEVELS_H_
