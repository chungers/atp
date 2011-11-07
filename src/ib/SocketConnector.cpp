
#include <boost/scoped_ptr.hpp>

#include "ib/AbstractSocketConnector.hpp"
#include "ib/ApiMessageBase.hpp"
#include "ib/Message.hpp"
#include "ib/ReactorStrategyFactory.hpp"

#include "zmq/ZmqUtils.hpp"


namespace IBAPI {

using ib::internal::AbstractSocketConnector;
using ib::internal::AsioEClientSocket;
using ib::internal::EWrapperFactory;
using ib::internal::EClientPtr;
using ib::internal::ReactorStrategyFactory;
using ib::internal::ReactorStrategy;
using ib::internal::ZmqMessage;


class SocketConnector::implementation :
      public ib::internal::AbstractSocketConnector
{
 public:
  implementation(const std::string& zmqInboundAddr,
                 Application& app, int timeout) :
      AbstractSocketConnector(zmqInboundAddr, app, timeout),
      reactorStrategyPtr_(ReactorStrategyFactory::getInstance())
  {
  }

  ~implementation()
  {
  }

 protected:
  virtual bool handleReactorInboundMessages(
      zmq::socket_t& socket, EClientPtr eclient)
  {
    bool status = false;
    int seq = 0;
    ib::internal::ZmqMessage inboundMessage;
    try {
      while (1) {
        if (inboundMessage.receive(socket)) {
          status = reactorStrategyPtr_->handleInboundMessage(
              inboundMessage, eclient);
          if (!status) {
            break;
          }
        }
      }
    } catch (zmq::error_t e) {
      LOG(WARNING) << "Got exception: " << e.what() << std::endl;
      status = false;
    }
    return status;
  }

  virtual zmq::socket_t* createOutboundSocket(int channel = 0)
  {
    return NULL;
  }

  friend class SocketConnector;
  boost::scoped_ptr<ReactorStrategy> reactorStrategyPtr_;
};



SocketConnector::SocketConnector(const std::string& zmqInboundAddr,
                                 Application& app, int timeout) :
    impl_(new SocketConnector::implementation(zmqInboundAddr, app, timeout))
{
  IBAPI_SOCKET_CONNECTOR_LOGGER << "SocketConnector started.";
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
  IBAPI_SOCKET_CONNECTOR_LOGGER << "CONNECT " << host << ":" << port;
  return impl_->connect(host, port, clientId, strategy);
}

} // namespace IBAPI

