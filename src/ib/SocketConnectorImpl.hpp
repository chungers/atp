#ifndef IB_SOCKET_CONNECTOR_IMPL_H_
#define IB_SOCKET_CONNECTOR_IMPL_H_

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

#include "ib/Application.hpp"
#include "ib/AsioEClientSocket.hpp"
#include "ib/EWrapperFactory.hpp"
#include "ib/SocketConnector.hpp"

#include "zmq/Reactor.hpp"
#include "zmq/ZmqUtils.hpp"


#define CONNECTOR_IMPL_LOGGER VLOG(VLOG_LEVEL_IBAPI_SOCKET_CONNECTOR_IMPL)
#define CONNECTOR_IMPL_WARNING LOG(WARNING)

using namespace IBAPI;

namespace ib {
namespace internal {



class SocketConnectorImpl :
      public atp::zmq::Reactor::Strategy,
      public ib::internal::EWrapperEventCollector,
      public ib::internal::AsioEClientSocket::EventCallback
{

 public:
  SocketConnectorImpl(const string& zmqInboundAddress,
                      Application& app, int timeout) :
      app_(app),
      timeoutSeconds_(timeout),
      reactor_(zmqInboundAddress, *this),
      reactorAddress_(zmqInboundAddress),
      socketConnector_(NULL)
  {
  }

  virtual ~SocketConnectorImpl()
  {
    if (socket_.get() != 0 || outboundSocket_.get() != 0) {
      CONNECTOR_IMPL_LOGGER << "Shutting down " << stop() << std::endl;
    }
  }

  /// @see AsioEClientSocket::EventCallback
  void onEventThreadStart()
  {
    // on successful connection. here we connect to the outbound socket
    // for publishing events.
    // this must be from the same thread as the thread in the socket event
    // dispatcher.
    zmq::socket_t* sink = createOutboundSocket();

    CONNECTOR_IMPL_LOGGER
        << "Creating event sink zmq socket:" << sink << std::endl;

    outboundSocket_.reset(sink);
  }

  /// @see AsioEClientSocket::EventCallback
  void onEventThreadStop()
  {
    CONNECTOR_IMPL_LOGGER << "EClient socket stopped." << std::endl;
  }

  /// @see AsioEClientSocket::EventCallback
  void onSocketConnect(bool success)
  {
    CONNECTOR_IMPL_LOGGER
        << "EClient socket connected:" << success << std::endl;
  }


  /// @see AsioEClientSocket::EventCallback
  void onSocketClose(bool success)
  {
    CONNECTOR_IMPL_LOGGER
        << "EClient socket closed:" << success << std::endl;
  }

  /// @see EWrapperEventCollector
  zmq::socket_t* getOutboundSocket()
  {
    return outboundSocket_.get();
  }

  /// @see Reactor::Strategy
  int socketType() { return ZMQ_REP; }

  /// @see Reactor::Strategy
  /// This method is run from the Reactor's thread.
  bool respond(zmq::socket_t& socket)
  {
    if (socket_.get() == 0 || !socket_->isConnected()) {
      return true; // No-op -- don't read the messages from socket yet.
    }

    // Now we are connected.  Process the received messages.
    return readSocketAndProcess(socket);
  }

  /// @overload
  int connect(const string& host,
              unsigned int port,
              unsigned int clientId,
              SocketConnector::Strategy* strategy)
  {
    // Check on the state.
    if (socket_.get() != 0 && socket_->isConnected()) {
        CONNECTOR_IMPL_WARNING
            << "Calling eConnect on already live connection."
            << std::endl;
        return socket_->getClientId();
    }

    EWrapper* ew = EWrapperFactory::getInstance(app_, *this, clientId);

    assert (ew != NULL);

    // Start a new socket.
    boost::lock_guard<boost::mutex> lock(mutex_);

    // If shared pointer hasn't been set, alloc new socket.
    if (socket_.get() == 0) {
      socket_ = boost::shared_ptr<AsioEClientSocket>(
          new AsioEClientSocket(ioService_, *ew, this));
    }

    int64 start = now_micros();
    socket_->eConnect(host.c_str(), port, clientId);

    // Spin until connected.  This is the part where some handshake
    // occurs with the IB gateway.
    int64 limit = timeoutSeconds_ * 1000000;
    while (!socket_->isConnected() && now_micros() - start < limit) {}

    if (socket_->isConnected()) {
      int64 elapsed = now_micros() - start;
      CONNECTOR_IMPL_LOGGER
          << "Connected in " << elapsed << " microseconds." << std::endl;

      strategy->onConnect(*socketConnector_, clientId);
      return clientId;
    } else {
      strategy->onTimeout(*socketConnector_);
      return -1;
    }
  }

  bool stop()
  {
    if (socket_.get() != 0) {
      socket_->eDisconnect();
      for (int i = 0; i < timeoutSeconds_ && socket_->isConnected(); ++i) {
        CONNECTOR_IMPL_LOGGER
            << "Waiting for EClientSocket to stop." << std::endl;
        sleep(1);
      }
    }
    socket_.reset();
    if (outboundSocket_.get() != 0) {
      CONNECTOR_IMPL_LOGGER << "Stopping publish socket." << std::endl;
      delete outboundSocket_.get();
    }
    outboundSocket_.reset();
    bool status = outboundSocket_.get() == 0 && socket_.get() == 0;
    CONNECTOR_IMPL_LOGGER
        << "Stopped all sockets (EClient + publish):" << status << std::endl;
    return status;
  }

 protected:
  virtual bool readSocketAndProcess(zmq::socket_t& socket)
  {
    // TODO
    LOG(WARNING) << "I shouldn't be here." << std::endl;
    return true;
  }


  virtual zmq::socket_t* createOutboundSocket()
  {
    LOG(WARNING) << "I shouldn't be here." << std::endl;
    return NULL;
  }

 private:

  Application& app_;
  int timeoutSeconds_;
  boost::asio::io_service ioService_; // Dedicated per connector
  boost::shared_ptr<AsioEClientSocket> socket_;
  boost::shared_ptr<boost::thread> thread_;
  boost::mutex mutex_;

  // For handling inbound requests.
  atp::zmq::Reactor reactor_;
  const std::string& reactorAddress_;

  boost::thread_specific_ptr<zmq::socket_t> outboundSocket_;

 protected:
  SocketConnector* socketConnector_;
};

} // namespace internal
} // namespace ib

#endif  //IB_SOCKET_CONNECTOR_IMPL_H_
