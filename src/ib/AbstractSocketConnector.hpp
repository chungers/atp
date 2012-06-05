#ifndef IB_ABSTRACT_SOCKET_CONNECTOR_H_
#define IB_ABSTRACT_SOCKET_CONNECTOR_H_

#include <sstream>
#include <sys/select.h>
#include <sys/time.h>

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

#include "varz/varz.hpp"

#include "zmq/Reactor.hpp"
#include "zmq/ZmqUtils.hpp"


#define CONNECTOR_IMPL_WARNING LOG(WARNING)

using namespace IBAPI;

DEFINE_VARZ_int64(socket_connector_connection_retries, 0, "");
DEFINE_VARZ_int64(socket_connector_connection_timeouts, 0, "");

namespace ib {
namespace internal {


class AbstractSocketConnector :
      public atp::zmq::Reactor::Strategy,
      // public ib::internal::EWrapperEventCollector,
      IBAPI::OutboundChannels,
      public ib::internal::AsioEClientDriver::EventCallback
{

 public:

  AbstractSocketConnector(
      const int reactorSocketType,
      const SocketConnector::ZmqAddress& reactorAddress,
      const SocketConnector::ZmqAddressMap& outboundChannels,
      Application& app, int timeout,
      zmq::context_t* inboundContext = NULL,
      zmq::context_t* outboundContext = NULL) :

      app_(app),
      timeoutSeconds_(timeout),
      reactorSocketType_(reactorSocketType),
      reactorAddress_(reactorAddress),
      reactor_(reactorSocketType, reactorAddress, *this, inboundContext),
      outboundChannels_(outboundChannels),
      outboundContext_(outboundContext),
      dispatcher_(NULL),
      socketConnector_(NULL)
  {
  }

  ~AbstractSocketConnector()
  {
    // if (driver_.get() != 0 || outboundSocket_.get() != 0) {
    //   IBAPI_ABSTRACT_SOCKET_CONNECTOR_LOGGER << "Shutting down " << stop();
    // }
    if (dispatcher_ != NULL) {
      delete dispatcher_;
    }
  }


 public:
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

  /// @see Reactor::Strategy
  /// This method is run from the Reactor's thread.
  bool respond(zmq::socket_t& socket)
  {
    if (driver_.get() == 0 || !driver_->IsConnected()) {
      return true; // No-op -- don't read the messages from socket yet.
    }
    // Now we are connected.  Process the received messages.
    return handleReactorInboundMessages(socket, driver_->GetEClient());
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
        CONNECTOR_IMPL_WARNING
            << "Calling eConnect when already connected.";

        return driver_->getClientId();
    }

    //EWrapper* ew = EWrapperFactory::getInstance(app_, *this, clientId);
    // ib::EWrapperPtr ew = app_.GetEWrapper(clientId, *this);

    dispatcher_ = app_.GetApiEventDispatcher(clientId);

    assert(dispatcher_ != NULL);

    dispatcher_->SetOutboundSockets(*this);
    EWrapper* ew = dispatcher_->GetEWrapper();

    assert(ew != NULL);

    // Start a new socket.
    boost::lock_guard<boost::mutex> lock(mutex_);

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

  bool stop(bool blockForReactor = false)
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
    // Wait for the reactor to stop -- this potentially can block forever
    // if the reactor doesn't not exit out of its processing loop.
    if (blockForReactor) {
      reactor_.block();
    }

    return true;
  }

 protected:

  Application& GetApplication()
  {
    return app_;
  }

  const int GetReactorSocketType()
  {
    return reactorSocketType_;
  }

  /**
   * Process messages from the socket and return true if ok.  This
   * is part of the Reactor implementation that handles any inbound
   * control messages (e.g. market data requests, orders, etc.)
   */
  virtual bool handleReactorInboundMessages(
      zmq::socket_t& socket, EClientPtr eclient) = 0;


 private:

  Application& app_;
  int timeoutSeconds_;
  boost::asio::io_service ioService_; // Dedicated per connector
  boost::shared_ptr<ApiProtocolHandler> protocolHandler_;
  boost::shared_ptr<AsioEClientDriver> driver_;

  boost::shared_ptr<boost::thread> thread_;
  boost::mutex mutex_;

  // For handling inbound requests.
  const int reactorSocketType_;
  const SocketConnector::ZmqAddress& reactorAddress_;
  atp::zmq::Reactor reactor_;

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

#endif  //IB_ABSTRACT_SOCKET_CONNECTOR_H_
