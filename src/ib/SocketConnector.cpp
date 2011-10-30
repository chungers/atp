
#include "ib/AbstractSocketConnector.hpp"

#define LOGGER VLOG(VLOG_LEVEL_IBAPI_SOCKET_CONNECTOR)

namespace IBAPI {

using ib::internal::AbstractSocketConnector;
using ib::internal::AsioEClientSocket;
using ib::internal::EWrapperFactory;

class SocketConnector::implementation :
      public ib::internal::AbstractSocketConnector
{
 public:
  implementation(const std::string& zmqInboundAddr,
                 Application& app, int timeout) :
      AbstractSocketConnector(zmqInboundAddr, app, timeout)
  {
  }

  ~implementation() {}

 protected:
  virtual bool handleReactorInboundMessages(
      zmq::socket_t& socket, EClientSocketPtr eclient)
  {
    return false;
  }

  virtual zmq::socket_t* createOutboundSocket(int channel = 0)
  {
    return NULL;
  }

  friend class SocketConnector;
};



SocketConnector::SocketConnector(const std::string& zmqInboundAddr,
                                 Application& app, int timeout) :
    impl_(new SocketConnector::implementation(zmqInboundAddr, app, timeout))
{
  LOGGER << "SocketConnector started.";
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

