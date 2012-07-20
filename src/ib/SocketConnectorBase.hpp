#ifndef IB_SOCKET_CONNECTOR_BASE_H_
#define IB_SOCKET_CONNECTOR_BASE_H_

#include <gflags/gflags.h>

#include <boost/asio.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/tss.hpp>
#include <zmq.hpp>

#include "log_levels.h"
#include "utils.hpp"

#include "ib/internal.hpp"
#include "ib/ApiEventDispatcher.hpp"
#include "ib/Application.hpp"
#include "ib/AsioEClientDriver.hpp"
#include "ib/SocketConnector.hpp"
#include "ib/ZmqMessage.hpp"

#include "varz/varz.hpp"

#include "zmq/Reactor.hpp"
#include "zmq/ZmqUtils.hpp"



using namespace IBAPI;

DEFINE_VARZ_int64(socket_connector_connection_retries, 0, "");
DEFINE_VARZ_int64(socket_connector_connection_timeouts, 0, "");
DEFINE_VARZ_int64(socket_connector_instances, 0, "");
DEFINE_VARZ_int64(socket_connector_inbound_requests, 0, "");
DEFINE_VARZ_int64(socket_connector_inbound_requests_ok, 0, "");
DEFINE_VARZ_int64(socket_connector_inbound_requests_errors, 0, "");
DEFINE_VARZ_int64(socket_connector_inbound_requests_exceptions, 0, "");

namespace ib {
namespace internal {


class SocketConnectorBase :
      IBAPI::OutboundChannels,
      public atp::zmq::Reactor::Strategy,
      public ib::internal::AsioEClientDriver::EventCallback
{

 public:

  explicit SocketConnectorBase(
      Application& app, int timeout,
      const SocketConnector::ZmqAddress& reactorAddress, int reactorSocketType,
      const SocketConnector::ZmqAddressMap& outboundChannels,
      zmq::context_t* inboundContext = NULL,
      zmq::context_t* outboundContext = NULL) :
      app_(app),
      timeoutSeconds_(timeout),
      reactorAddress_(reactorAddress),
      reactorSocketType_(reactorSocketType),
      inboundContext_(inboundContext),
      outboundChannels_(outboundChannels),
      outboundContext_(outboundContext),
      dispatcher_(NULL),
      socketConnector_(NULL)
  {
  }

  ~SocketConnectorBase()
  {
    if (dispatcher_ != NULL) {
      delete dispatcher_;
    }
    if (reactor_ != NULL) {
      delete reactor_;
    }
  }


 public:

  const int GetReactorSocketType() const
  {
    return reactorSocketType_;
  }

  void start()
  {
    reactor_ = new atp::zmq::Reactor(
        reactorSocketType_, reactorAddress_, *this, inboundContext_);
  }

  /// @see AsioEClientDriver::EventCallback
  void onEventThreadStart()
  {
    // on successful connection. here we connect to the outbound sockets
    // for publishing events in different channels.
    // this must be from the same thread as the thread in the socket event
    // dispatcher.

    // Start the zmq context if not provided.
    IBAPI_ABSTRACT_SOCKET_CONNECTOR_LOGGER
        << "Outbound context = " << outboundContext_;

    if (outboundContext_ == NULL) {
      outboundContext_ = new zmq::context_t(1);
      IBAPI_ABSTRACT_SOCKET_CONNECTOR_LOGGER
          << "Starting local context: " << outboundContext_;
    }

    SocketConnector::ZmqAddressMap::const_iterator channelAddress =
        outboundChannels_.begin();

    // Simply loop through the map of channels and addresses and
    // create a socket for each channel.  There is no check to see
    // if the socket address are the same and sharing of sockets.
    // This should be fine as these are PUSH sockets so multiple sockets
    // connecting to the same address is permissible.
    for (; channelAddress != outboundChannels_.end(); ++channelAddress) {

      int channel = channelAddress->first;
      SocketConnector::ZmqAddress address = channelAddress->second;

      zmq::socket_t* outbound = new zmq::socket_t(*outboundContext_, ZMQ_PUSH);

      int retry = 0;
      do {

        try {

          IBAPI_ABSTRACT_SOCKET_CONNECTOR_LOGGER
              << "Channel " << channel << ": "
              << "Connecting to " << address;

          outbound->connect(address.c_str());

          if (outboundSockets_.get() == NULL) {
            IBAPI_ABSTRACT_SOCKET_CONNECTOR_LOGGER
                << "Creating outbound socket map.";
            outboundSockets_.reset(new SocketMap);
          }
          (*outboundSockets_)[channel] = outbound;

          IBAPI_ABSTRACT_SOCKET_CONNECTOR_LOGGER
              << "Channel " << channel << ": "
              << "ZMQ_PUSH socket:" << outbound << " ready.";

          retry = 0;
        } catch (zmq::error_t e) {
          LOG(WARNING)
              << "Channel " << channel << ": "
              << "Unable to connecto to " << address << " using context "
              << outboundContext_ << ": "
              << e.what();

          retry++;
          sleep(1);
        }
      }
      // Only up to 5 seconds.  This is part of the connection negotiation
      // with the gateway.  Therefore, if we take too long, the gateway
      // thinks the client is down.
      while (retry > 0 && retry < 5);

      if (retry != 0) {
        // We failed to connect after so many attempts
        LOG(ERROR)
            << "Channel " << channel << ": "
            << "Cannot connect to outbound destination after "
            << retry << " attempts. Logging events only.";
      }
    }
  }

  /// @see AsioEClientDriver::EventCallback
  void onEventThreadStop()
  {
    IBAPI_ABSTRACT_SOCKET_CONNECTOR_LOGGER << "Closing outbound sockets.";
    // Need to delete the outbound sockets here since this is running in the
    // same thread as onEventThreadStart() which created the outbound sockets.
    if (outboundSockets_.get() != NULL) {
      SocketMap::iterator channelPair = outboundSockets_->begin();
      for (; channelPair != outboundSockets_->end(); ++channelPair) {
        IBAPI_ABSTRACT_SOCKET_CONNECTOR_LOGGER
            << "Closing socket for channel " << channelPair->first;
        delete channelPair->second;
      }
    }
  }

  /// @see AsioEClientDriver::EventCallback
  void onSocketConnect(bool success)
  {
    IBAPI_ABSTRACT_SOCKET_CONNECTOR_LOGGER
        << "EClient socket connected:" << success;
  }


  /// @see AsioEClientDriver::EventCallback
  void onSocketClose(bool success)
  {
    IBAPI_ABSTRACT_SOCKET_CONNECTOR_LOGGER
        << "EClient socket closed:" << success;
  }

  /// @see EWrapperEventCollector
  zmq::socket_t* getOutboundSocket(unsigned int channel = 0)
  {
    if (outboundSockets_.get() != NULL &&
        outboundSockets_->find(channel) != outboundSockets_->end()) {
      return (*outboundSockets_)[channel];
    }
    return NULL;
  }

  /// @overload
  int connect(const string& host,
              unsigned int port,
              unsigned int clientId,
              SocketConnector::Strategy* strategy,
              int maxAttempts = 60)
  {
    // Check on the state.
    if (driver_.get() != 0 && driver_->IsConnected()) {
        IBAPI_SOCKET_CONNECTOR_WARNING
            << "Calling eConnect when already connected.";

        return driver_->getClientId();
    }

    // Start a new socket.
    boost::lock_guard<boost::mutex> lock(mutex_);

    dispatcher_ = app_.GetApiEventDispatcher(clientId);

    assert(dispatcher_ != NULL);

    dispatcher_->SetOutboundSockets(*this);
    EWrapper* ew = dispatcher_->GetEWrapper();

    assert(ew != NULL);

    // If shared pointer hasn't been set, alloc new socket.
    if (driver_.get() == 0) {
      protocolHandler_ = boost::shared_ptr<ApiProtocolHandler>(
          new ApiProtocolHandler(*ew));
      driver_ = boost::shared_ptr<AsioEClientDriver>(
          new AsioEClientDriver(ioService_, *protocolHandler_, this));
    }

    for (int attempts = 0;
         attempts < maxAttempts;
         ++attempts, sleep(1)) {

      LOG(ERROR) << "About to start ";
      int64 start = now_micros();

      LOG(ERROR) << "About to connect " << start << ", clientId = " << clientId;
      bool socketOk = driver_->Connect(host.c_str(), port, clientId);

      if (socketOk) {
        // Spin until connected -- including the part where some handshake
        // occurs with the IB gateway.
        int64 limit = timeoutSeconds_ * 1000000;
        while (!driver_->IsConnected() && now_micros() - start < limit) { }

        if (driver_->IsConnected()) {

          int64 elapsed = now_micros() - start;
          IBAPI_ABSTRACT_SOCKET_CONNECTOR_LOGGER
              << "Connected in " << elapsed << " microseconds.";

          strategy->onConnect(*socketConnector_, clientId);
          return clientId;

        }
      } else {

        driver_->reset();
        IBAPI_ABSTRACT_SOCKET_CONNECTOR_LOGGER
            << "Unable to connect to gateway. Attempts = " << attempts;
      }

      VARZ_socket_connector_connection_retries++;
    }

    // When we reached here, we have exhausted all attempts:
    VARZ_socket_connector_connection_timeouts++;
    strategy->onTimeout(*socketConnector_);

    return -1;
  }

  bool stop(bool blockForCleanStop = false)
  {
    if (driver_.get() != 0) {
      IBAPI_ABSTRACT_SOCKET_CONNECTOR_LOGGER << "Disconnecting from gateway.";
      driver_->Disconnect();
      for (int i = 0; driver_->IsConnected(); ++i) {
        IBAPI_ABSTRACT_SOCKET_CONNECTOR_LOGGER
            << "Waiting for EClientSocket to stop.";
        sleep(1);
        if (i > timeoutSeconds_) {
          IBAPI_ABSTRACT_SOCKET_CONNECTOR_LOGGER
              << "Timed out waiting for EClientSocket to stop.";
          break;
        }
      }
      driver_.reset();
    }
    if (blockForCleanStop) {
      BlockUntilCleanStop();
    }

    return true;
  }

  /// @see Reactor::Strategy
  /// This method is run from the Reactor's thread.
  virtual bool respond(zmq::socket_t& socket)
  {
    if (!IsDriverReady()) {
      return true; // No-op -- don't read the messages from socket yet.
    }
    // Now we are connected.  Process the received messages.
    return handleReactorInboundMessages(socket, GetEClient());
  }

 protected:

  /// After the reactor message has been received, parsed and / or processed.
  virtual void afterMessage(unsigned int responseCode,
                            zmq::socket_t& socket,
                            ib::internal::ZmqMessagePtr& origMessageOptional)
  {
    if (GetReactorSocketType() == ZMQ_REP) {
      atp::zmq::send_copy(socket, boost::lexical_cast<string>(responseCode));
    }
  }

  virtual void BlockUntilCleanStop()
  {
    reactor_->block();
  }

  bool IsDriverReady()
  {
    return (driver_.get() != 0 && !driver_->IsConnected());
  }

  EClientPtr GetEClient()
  {
    return driver_->GetEClient();
  }

  Application& GetApplication()
  {
    return app_;
  }

  /**
   * Process messages from the socket and return true if ok.  This
   * is part of the Reactor implementation that handles any inbound
   * control messages (e.g. market data requests, orders, etc.)
   */
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

 private:

  Application& app_;
  int timeoutSeconds_;
  boost::asio::io_service ioService_; // Dedicated per connector
  boost::shared_ptr<ApiProtocolHandler> protocolHandler_;
  boost::shared_ptr<AsioEClientDriver> driver_;

  boost::shared_ptr<boost::thread> thread_;
  boost::mutex mutex_;

  // For inbound messages
  const SocketConnector::ZmqAddress& reactorAddress_;
  const int reactorSocketType_;
  zmq::context_t* inboundContext_;
  atp::zmq::Reactor* reactor_;

  // For outbound messages
  const SocketConnector::ZmqAddressMap& outboundChannels_;

  typedef std::map< int, zmq::socket_t* > SocketMap;
  boost::thread_specific_ptr<SocketMap> outboundSockets_;

  zmq::context_t* outboundContext_;

  IBAPI::ApiEventDispatcher* dispatcher_;

 protected:
  SocketConnector* socketConnector_;
};

} // namespace internal
} // namespace ib

#endif  //IB_SOCKET_CONNECTOR_BASE_H_
