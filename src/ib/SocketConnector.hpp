#ifndef IBAPI_SOCKET_CONNECTOR_H_
#define IBAPI_SOCKET_CONNECTOR_H_

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

  SocketConnector(const std::string& zmqInboundAddr,
                  const std::string& zmqOutboundAddr,
                  Application& app, int timeout = 0,
                  zmq::context_t* context = NULL);
  ~SocketConnector();

  /// Blocking connect, up to the timeout limit in seconds.
  int connect(const std::string& host, unsigned int port, unsigned int clientId,
              Strategy* s, int maxAttempts = 60);

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
        << "onConnect(" << clientId << ")" << std::endl;
  }

  void onData(SocketConnector& sc, int clientId)
  {
    IBAPI_SOCKET_CONNECTOR_STRATEGY_LOGGER
        << "onData(" << clientId << ")" << std::endl;
  }

  void onDisconnect(SocketConnector& sc, int clientId)
  {
    IBAPI_SOCKET_CONNECTOR_STRATEGY_LOGGER
        << "onDisconnect(" << clientId << ")" << std::endl;
  }

  void onError(SocketConnector& sc)
  {
    LOG(ERROR) << "onError" << std::endl;
  }

  void onTimeout(SocketConnector& sc)
  {
    LOG(ERROR) << "onTimeout" << std::endl;
  }
};

} // namespace IBAPI


#endif // IBAPI_SOCKET_CONNECTOR_H_

