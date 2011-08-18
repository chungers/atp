
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

class SocketConnectorImpl {

 public:
  SocketConnectorImpl(Application& app, int timeout) :
      app_(app),
      timeoutSeconds_(timeout),
      socket_(NULL),
      socketConnector_(NULL)
  {
  }
  
  ~SocketConnectorImpl()
  {
    if (socket_ != NULL) {
      socket_->eDisconnect();
      for (int i = 0; i < timeoutSeconds_ && socket_->isConnected(); ++i) {
        sleep(1);
      }
      delete socket_;
    }
  }

  /// @overload
  int connect(const string& host,
              unsigned int port,
              unsigned int clientId,
              SocketConnector::Strategy* strategy)
  {
    LOG(INFO) << "Connecting..." << std::endl;

    EWrapper* ew = EWrapperFactory::getInstance()->getImpl(app_, *strategy, clientId);
    assert (ew != NULL);

    if (socket_ != NULL) {
      if (socket_->isConnected()) {
        LOG(WARNING) << "Calling eConnect on already live connection." << std::endl;
        return socket_->getClientId();
      } else {
        delete socket_;
      }
    }
    
    // Start a new socket.
    socket_ = new AsioEClientSocket(ioService_, *ew);
    socket_->eConnect(host.c_str(), port, clientId);
    
    for (int seconds = 0; !socket_->isConnected() && seconds < timeoutSeconds_; ++seconds) {
      VLOG(VLOG_LEVEL_IBAPI_SOCKET_CONNECTOR) << "Waiting for connection : "
                                              << seconds << " seconds " << std::endl;
      sleep(1);
    }
    
    if (socket_->isConnected()) {
      strategy->onConnect(*socketConnector_, clientId);
      return clientId;
    } else {
      strategy->onTimeout(*socketConnector_);
      return -1;
    }
  }

  
 private:
  Application& app_;
  int timeoutSeconds_;
  boost::asio::io_service ioService_; // Dedicated per connector
  AsioEClientSocket* socket_;
  
 protected:
  SocketConnector* socketConnector_;
  
};



class SocketConnector::implementation : public SocketConnectorImpl {
 public:
  implementation(Application& app, int timeout) :
      SocketConnectorImpl(app, timeout) {}

  ~implementation() {}

  friend class SocketConnector;
};



SocketConnector::SocketConnector(Application& app, int timeout)
    : impl_(new SocketConnector::implementation(app, timeout))
{
    VLOG(VLOG_LEVEL_IBAPI_SOCKET_CONNECTOR) << "SocketConnector starting." << std::endl;
    impl_->socketConnector_ = this;
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
  LOG(INFO) << "CONNECT" << std::endl;
  return impl_->connect(host, port, clientId, strategy);
}


} // namespace IBAPI
