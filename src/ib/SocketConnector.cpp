
#include <boost/scoped_ptr.hpp>

#include "ib/AbstractSocketConnector.hpp"
#include "ib/ZmqMessage.hpp"

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
using ib::internal::ZmqMessage;

using boost::scoped_ptr;

class SocketConnector::implementation : public AbstractSocketConnector
{
 public:
  implementation(const ZmqAddress& reactorAddress,
                 const ZmqAddressMap& outboundChannels,
                 Application& app, int timeout,
                 zmq::context_t* inboundContext = NULL,
                 zmq::context_t* outboundContext = NULL) :
      AbstractSocketConnector(reactorAddress, outboundChannels,
                              app, timeout, inboundContext, outboundContext)
  {
    VARZ_socket_connector_instances++;
  }

  ~implementation()
  {
  }

  /// Reactor's socket type -- req/reply
  virtual int socketType()
  {
    return ZMQ_REP;
  }

 protected:

  virtual void afterMessage(unsigned int responseCode,
                            zmq::socket_t& socket,
                            ib::internal::ZmqMessagePtr& origMessageOptional)
  {
    if (socketType() == ZMQ_REP) {
      atp::zmq::send_copy(socket, boost::lexical_cast<string>(responseCode));
    }
  }

  virtual bool handleReactorInboundMessages(zmq::socket_t& socket,
                                            EClientPtr eclient)
  {
    using ib::internal::ZmqMessagePtr;

    try {

      while (1) {

        std::string messageKeyFrame;
        bool more = atp::zmq::receive(socket, &messageKeyFrame);
        bool supported = GetApplication().IsMessageSupported(messageKeyFrame);

        if (!supported) {

          IBAPI_SOCKET_CONNECTOR_ERROR << "Unsupported message received: "
                                       << messageKeyFrame;
          // keep reading to consume the extra frames:
          while (more) {
            std::string buff;
            more = atp::zmq::receive(socket, &buff);
            IBAPI_SOCKET_CONNECTOR_ERROR << "Unsupported message received: "
                                         << messageKeyFrame
                                         << ", extra frame: " << buff;
          }

          ZmqMessagePtr empty;
          afterMessage(404, socket, empty);

          // now skip this loop
          continue;
        }

        ZmqMessagePtr inboundMessage;
        int responseCode = 500;

        // Message is supported.  Now create the message and delegate it
        // to the actual reading and parsing from the socket.
        if (more) {

          LOG(INFO) << "Received message of type " << messageKeyFrame;

          ZmqMessage::createMessage(messageKeyFrame, inboundMessage);

          if (inboundMessage && (*inboundMessage)->receive(socket)) {

            VARZ_socket_connector_inbound_requests++;

            if (!(*inboundMessage)->validate()) {

              VARZ_socket_connector_inbound_requests_errors++;

              responseCode = 412; // pre conditional failed

              IBAPI_SOCKET_CONNECTOR_ERROR
                  << "Handle inbound message failed: "
                  << inboundMessage;

            } else {

              if ((*inboundMessage)->callApi(eclient)) {

                VARZ_socket_connector_inbound_requests_ok++;

                responseCode = 200;

              } else {

                VARZ_socket_connector_inbound_requests_errors++;

                responseCode = 502; // bad gateway

                // TODO: figure out a better way to log something
                // helpful - like a message type identifier.
                IBAPI_SOCKET_CONNECTOR_ERROR
                    << "Handle inbound message failed: "
                    << inboundMessage;

              }
            }
          } else {
            responseCode = (inboundMessage) ?
                400 : // bad request
                503; // service unavailable
          }
        } // if more

        // Process response after message handled.
        afterMessage(responseCode, socket, inboundMessage);
      }
    } catch (zmq::error_t e) {

      VARZ_socket_connector_inbound_requests_exceptions++;

      LOG(ERROR) << "Got exception while handling reactor inbound message: "
                 << e.what();

      return false; // stop processing
    }
    return true; // continue processing
  }

  friend class SocketConnector;
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

