#ifndef IBAPI_SOCKET_CONNECTOR_H_
#define IBAPI_SOCKET_CONNECTOR_H_

#include <map>
#include <zmq.hpp>
#include <boost/scoped_ptr.hpp>

#include "common.hpp"
#include "log_levels.h"
#include "ib/Application.hpp"


namespace IBAPI {

/// Models after the SocketConnector in the QuickFIX API.
/// with extensions of using zmq for inbound (control) messages
/// and outbound event collection.
/// https://github.com/lab616/third_party/blob/master/quickfix-1.13.3/src/C++/SocketConnector.h
class SocketConnector : NoCopyAndAssign {

 public:
  class Strategy;

  typedef std::string ZmqAddress;
  typedef std::map<int, ZmqAddress > ZmqAddressMap;

  SocketConnector(const ZmqAddress& reactorAddress,
                  const ZmqAddressMap& outboundChannels,
                  Application& app, int timeout = 0,
                  zmq::context_t* inboundContext = NULL,
                  zmq::context_t* outboundContext = NULL);
  ~SocketConnector();

  bool start();

  /// Blocking connect, up to the timeout limit in seconds.
  int connect(const std::string& host, unsigned int port, unsigned int clientId,
              Strategy* s, int maxAttempts = 60);

  bool stop();

 private :
  class implementation;
  boost::scoped_ptr<implementation> impl_;

 public:
  class Strategy {
   public:
    virtual ~Strategy() {}
    virtual void onConnect(SocketConnector&, int clientId) = 0;
    virtual void onData(SocketConnector&, int clientId) = 0;
    virtual void onDisconnect(SocketConnector&, int clientId) = 0;
    virtual void onError(SocketConnector&) = 0;
    virtual void onTimeout(SocketConnector&) = 0;
  };
};


class StrategyBase : public SocketConnector::Strategy
{
 public:
  StrategyBase() {}
  ~StrategyBase() {}

  void onConnect(SocketConnector& sc, int clientId)
  {
    IBAPI_SOCKET_CONNECTOR_STRATEGY_LOGGER
        << "SocketConnector(" << &sc << ")::"
        << "onConnect(" << clientId << ")";
  }

  void onData(SocketConnector& sc, int clientId)
  {
    IBAPI_SOCKET_CONNECTOR_STRATEGY_LOGGER
        << "SocketConnector(" << &sc << ")::"
        << "onData(" << clientId << ")";
  }

  void onDisconnect(SocketConnector& sc, int clientId)
  {
    IBAPI_SOCKET_CONNECTOR_STRATEGY_LOGGER
        << "SocketConnector(" << &sc << ")::"
        << "onDisconnect(" << clientId << ")";
  }

  void onError(SocketConnector& sc)
  {
    LOG(ERROR)
        << "SocketConnector(" << &sc << ")::"
        << "onError";
  }

  void onTimeout(SocketConnector& sc)
  {
    LOG(ERROR)
        << "SocketConnector(" << &sc << ")::"
        << "onTimeout";
  }
};

} // namespace IBAPI


#endif // IBAPI_SOCKET_CONNECTOR_H_

