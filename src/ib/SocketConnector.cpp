
#include <sys/select.h>
#include <sys/time.h>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include <boost/asio.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include "log_levels.h"

#include "ib/AsioEClientSocket.hpp"
#include "ib/EWrapperFactory.hpp"
#include "ib/SocketConnector.hpp"


using ib::internal::AsioEClientSocket;
using ib::internal::EWrapperFactory;

namespace IBAPI {

class SocketConnectorImpl : public SocketConnector {

 public:
  SocketConnectorImpl(int timeout) :
      timeoutSeconds_(timeout),
      ewrapperFactoryPtr_(EWrapperFactory::getInstance())
  {
  }
  
  ~SocketConnectorImpl() {}

  /// @overload
  int connect(const string& host,
              unsigned int port,
              unsigned int clientId,
              Strategy* strategy)
  {
    return 0; // TODO
  }
  
 private:
  int timeoutSeconds_;
  boost::shared_ptr<EWrapperFactory> ewrapperFactoryPtr_;
  boost::asio::io_service ioService_; /// TODO: singleton??
  boost::scoped_ptr<AsioEClientSocket> clientSocketPtr_;
};


SocketConnector::SocketConnector(int timeout)
    : impl_(new SocketConnectorImpl(timeout))
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
