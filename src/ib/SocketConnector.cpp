
#include <sys/select.h>
#include <sys/time.h>

#include <gflags/gflags.h>

#include <boost/asio.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include "log_levels.h"

#include "ib/Application.hpp"
#include "ib/AsioEClientSocket.hpp"
#include "ib/EWrapperFactory.hpp"
#include "ib/SocketConnector.hpp"


using ib::internal::AsioEClientSocket;
using ib::internal::EWrapperFactory;

namespace IBAPI {

class SocketConnectorImpl : public SocketConnector {

 public:
  SocketConnectorImpl(Application& app, Strategy& strategy, int timeout) :
      timeoutSeconds_(timeout),
      clientSocketPtr_(new AsioEClientSocket(
          ioService_,
          *(EWrapperFactory::getInstance()->getImpl(app, strategy))))
  {
  }
  
  ~SocketConnectorImpl() {}

  /// @overload
  int connect(const string& host,
              unsigned int port,
              unsigned int clientId,
              Strategy* strategy)
  {
    clientSocketPtr_->eConnect(host.c_str(), port, clientId);
    for (int seconds = 0; !clientSocketPtr_->isConnected() && seconds < timeoutSeconds_; ++seconds) {
      VLOG(VLOG_LEVEL_IBAPI_SOCKET_CONNECTOR) << "Waiting for connection : "
                                              << seconds << " seconds " << std::endl;
      sleep(1);
    }
    if (clientSocketPtr_->isConnected()) {
      strategy->onConnect(*this, clientId);
      return clientId;
    } else {
      strategy->onTimeout(*this);
      return -1;
    }
  }
  
 private:
  int timeoutSeconds_;
  boost::asio::io_service ioService_; // Dedicated per connector
  boost::scoped_ptr<AsioEClientSocket> clientSocketPtr_;
};


SocketConnector::SocketConnector(Application& app, Strategy& strategy, int timeout)
    : impl_(new SocketConnectorImpl(app, strategy, timeout))
{
    VLOG(VLOG_LEVEL_IBAPI_SOCKET_CONNECTOR) << "SocketConnector starting." << std::endl;
}

SocketConnector::~SocketConnector()
{
}


/// QuickFIX implementation returns the socket fd.  In our case, we simply
/// return the clientId.
int SocketConnector::connect(const string& host,
                             unsigned int port,
                             unsigned int clientId,
                             Strategy* strategy)
{
  return impl_->connect(host, port, clientId, strategy);
}


} // namespace IBAPI
