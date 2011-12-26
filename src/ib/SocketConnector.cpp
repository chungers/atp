
#include <boost/scoped_ptr.hpp>

#include "ib/AbstractSocketConnector.hpp"
#include "ib/ApiMessageBase.hpp"
#include "ib/Message.hpp"
#include "ib/ReactorStrategyFactory.hpp"

#include "varz/varz.hpp"
#include "zmq/ZmqUtils.hpp"


DEFINE_VARZ_int64(socket_connector_instances, 0, "");
DEFINE_VARZ_int64(socket_connector_inbound_requests, 0, "");
DEFINE_VARZ_int64(socket_connector_inbound_requests_ok, 0, "");
DEFINE_VARZ_int64(socket_connector_inbound_requests_errors, 0, "");
DEFINE_VARZ_int64(socket_connector_inbound_requests_exceptions, 0, "");

namespace IBAPI {

using ib::internal::AbstractSocketConnector;
using ib::internal::AsioEClientSocket;
using ib::internal::EWrapperFactory;
using ib::internal::EClientPtr;
using ib::internal::ReactorStrategyFactory;
using ib::internal::ReactorStrategy;
using ib::internal::ZmqMessage;


class SocketConnector::implementation : public AbstractSocketConnector
{
 public:
  implementation(const ZmqAddress& reactorAddress,
                 const ZmqAddressMap& outboundChannels,
                 Application& app, int timeout,
                 zmq::context_t* inboundContext = NULL,
                 zmq::context_t* outboundContext = NULL) :
      AbstractSocketConnector(reactorAddress, outboundChannels,
                              app, timeout, inboundContext, outboundContext),
      reactorStrategyPtr_(ReactorStrategyFactory::getInstance())
  {
    VARZ_socket_connector_instances++;
  }

  ~implementation()
  {
  }

 protected:
  virtual bool handleReactorInboundMessages(
      zmq::socket_t& socket, EClientPtr eclient)
  {
    bool status = false;

    try {
      while (1) {
        ib::internal::ZmqMessage inboundMessage;
        if (inboundMessage.receive(socket)) {

          VARZ_socket_connector_inbound_requests++;

          status = reactorStrategyPtr_->handleInboundMessage(
              inboundMessage, eclient);
          if (!status) {

            VARZ_socket_connector_inbound_requests_errors++;

            IBAPI_SOCKET_CONNECTOR_LOGGER
                << "Handle inbound message failed: "
                << inboundMessage;

            break;
          } else {

            VARZ_socket_connector_inbound_requests_ok++;

            // Send ok reply -- must write something on the ZMQ_REP
            // socket or an invalid state exception will be thrown by zmq.
            // For simplicity, just use HTTP status codes.
            atp::zmq::send_copy(socket, "200");
          }
        }
      }
      // If we broke out to here, an error occurred or handling failed.
      // HTTP error code - server side error 500.
      atp::zmq::send_copy(socket, "500");
    } catch (zmq::error_t e) {

      VARZ_socket_connector_inbound_requests_exceptions++;

      LOG(ERROR) << "Got exception while handling reactor inbound message: "
                 << e.what();
      status = false;
    }
    return status;
  }

  friend class SocketConnector;
  boost::scoped_ptr<ReactorStrategy> reactorStrategyPtr_;
};



SocketConnector::SocketConnector(const ZmqAddress& reactorAddress,
                                 const ZmqAddressMap& outboundChannels,
                                 Application& app, int timeout,
                                 ::zmq::context_t* inboundContext,
                                 ::zmq::context_t* outboundContext) :
    impl_(new SocketConnector::implementation(reactorAddress,
                                              outboundChannels, app, timeout,
                                              inboundContext, outboundContext))
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
                             Strategy* strategy,
                             int maxAttempts)
{
  return impl_->connect(host, port, clientId, strategy, maxAttempts);
}

bool SocketConnector::stop()
{
  IBAPI_SOCKET_CONNECTOR_LOGGER << "STOP";
  // By default we don't wait for the reactor to stop.  This is because
  // reactor will keep running in a loop and to properly stop it, we'd need
  // a protocol to tell the reactor to exit the running loop.  This isn't
  // worth the extra complexity.  Instead, just let the code continue to
  // exit and eventually the zmq context will be destroyed and force the
  // reactor loop to exit on exception.
  return impl_->stop(false);
}

} // namespace IBAPI

