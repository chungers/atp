
#include "ib/AbstractSocketConnector.hpp"
#include "ib/ZmqMessage.hpp"

#include "varz/varz.hpp"
#include "zmq/ZmqUtils.hpp"


namespace IBAPI {

using ib::internal::AbstractSocketConnector;

class SocketConnector::implementation : AbstractSocketConnector
{
 public:
  implementation(const ZmqAddress& reactorAddress,
                 const ZmqAddressMap& outboundChannels,
                 Application& app, int timeout,
                 zmq::context_t* inboundContext = NULL,
                 zmq::context_t* outboundContext = NULL) :
      AbstractSocketConnector(app, timeout,
                              outboundChannels,
                              outboundContext),

#ifdef SOCKET_CONNECTOR_USES_BLOCKING_REACTOR
      reactorSocketType_(ZMQ_REP),
#else
      reactorSocketType_(ZMQ_PULL),
#endif
      reactorAddress_(reactorAddress)
  {
    reactor_ = new atp::zmq::Reactor(
        reactorSocketType_, reactorAddress, *this, inboundContext);
  }

  ~implementation()
  {
    delete reactor_;
  }

  /// After the reactor message has been received, parsed and / or processed.
  virtual void afterMessage(unsigned int responseCode,
                            zmq::socket_t& socket,
                            ib::internal::ZmqMessagePtr& origMessageOptional)
  {
    if (GetReactorSocketType() == ZMQ_REP) {
      atp::zmq::send_copy(socket, boost::lexical_cast<string>(responseCode));
    }
  }

 protected:

  virtual const int GetReactorSocketType()
  {
    return reactorSocketType_;
  }

  virtual void ReactorBlock()
  {
    reactor_->block();
  }

 private:
  // For handling inbound requests.
  const int reactorSocketType_;
  const SocketConnector::ZmqAddress& reactorAddress_;
  atp::zmq::Reactor* reactor_;

  friend class IBAPI::SocketConnector;

};

SocketConnector::SocketConnector(const ZmqAddress& reactorAddress,
                                 const ZmqAddressMap& outboundChannels,
                                 Application& app, int timeout,
                                 ::zmq::context_t* inboundContext,
                                 ::zmq::context_t* outboundContext) :
    impl_(new SocketConnector::implementation(reactorAddress, outboundChannels,
                                              app, timeout,
                                              inboundContext,
                                              outboundContext))
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

