#ifndef IBAPI_SOCKET_CONNECTOR_H_
#define IBAPI_SOCKET_CONNECTOR_H_

#include <boost/scoped_ptr.hpp>

#include "common.hpp"

namespace IBAPI {

/// Models after the SocketConnector in the QuickFIX API.
/// https://github.com/lab616/third_party/blob/master/quickfix-1.13.3/src/C++/SocketConnector.h
class SocketConnector : NoCopyAndAssign {

 public:
  class Strategy;

  SocketConnector(int timeout = 0);
  ~SocketConnector();

  int connect(const std::string& host, unsigned int port, unsigned int clientId,
              Strategy* s);
  
 private :
  boost::scoped_ptr<SocketConnector> impl_;

 public:
  class Strategy {
   public:
    virtual ~Strategy() = 0;
    virtual void onConnect(SocketConnector&, int clientId) = 0;
    virtual void onData(SocketConnector&, int clientId) = 0;
    virtual void onDisconnect(SocketConnector&, int clientId) = 0;
    virtual void onError(SocketConnector&) = 0;
    virtual void onTimeout(SocketConnector&) = 0;
  };
};


} // namespace IBAPI


#endif // IBAPI_SOCKET_CONNECTOR_H_

