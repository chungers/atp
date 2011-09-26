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

#include "zmq/Responder.hpp"
#include "zmq/ZmqUtils.hpp"


#define CONNECTOR_IMPL_LOGGER VLOG(VLOG_LEVEL_IBAPI_SOCKET_CONNECTOR_IMPL)
#define CONNECTOR_IMPL_WARNING LOG(WARNING)

using namespace IBAPI;

namespace ib {
namespace internal {



class SocketConnectorImpl :
      public atp::zmq::Responder::Strategy,
      public ib::internal::EWrapperEventSink,
      public ib::internal::AsioEClientSocket::EventCallback
{

 public:
  SocketConnectorImpl(Application& app, int timeout,
                      const string& responderAddress) :
      app_(app),
      timeoutSeconds_(timeout),
      responder_(responderAddress, *this),
      responderAddress_(responderAddress),
      socketConnector_(NULL)
  {
  }

  ~SocketConnectorImpl()
  {
    CONNECTOR_IMPL_LOGGER << "Shutting down." << std::endl;
    if (socket_.get() != 0) {
      socket_->eDisconnect();
      for (int i = 0; i < timeoutSeconds_ && socket_->isConnected(); ++i) {
        sleep(1);
      }
    }
    socket_.reset();
    CONNECTOR_IMPL_LOGGER << "Shutting down -- completed." << std::endl;
  }

  /// @see AsioEClientSocket::EventCallback
  void onEventThreadStart()
  {
    LOG(INFO) << "Start the event sink zmq socket here." << std::endl;
  }

  /// @see AsioEClientSocket::EventCallback
  void onEventThreadStop()
  {

  }

  /// @see AsioEClientSocket::EventCallback
  void onSocketConnect(bool success)
  {

  }


  /// @see AsioEClientSocket::EventCallback
  void onSocketClose(bool success)
  {

  }

  /// @see EWrapperEventSink
  zmq::socket_t* getSink()
  {
    return NULL;
  }

  /// @see Responder::Strategy
  /// This method is run from the Responder's thread.
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

 protected:
  virtual bool readSocketAndProcess(zmq::socket_t& socket)
  {
    // TODO
    LOG(WARNING) << "I shouldn't be here." << std::endl;
    return true;
  }


 private:

  Application& app_;
  int timeoutSeconds_;
  boost::asio::io_service ioService_; // Dedicated per connector
  boost::shared_ptr<AsioEClientSocket> socket_;
  boost::shared_ptr<boost::thread> thread_;
  boost::mutex mutex_;
  atp::zmq::Responder responder_;
  const std::string& responderAddress_;

  //boost::thread_specific_ptr<zmq::socket_t> zmqEventSinkSocket_;

 protected:
  SocketConnector* socketConnector_;
};

} // namespace internal
} // namespace ib

#endif  //IB_SOCKET_CONNECTOR_IMPL_H_
