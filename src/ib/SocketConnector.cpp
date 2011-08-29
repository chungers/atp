
#include <sstream>
#include <sys/select.h>
#include <sys/time.h>

#include <gflags/gflags.h>

#include <boost/asio.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <zmq.hpp>

#include "log_levels.h"
#include "utils.hpp"

#include "ib/Application.hpp"
#include "ib/AsioEClientSocket.hpp"
#include "ib/EWrapperFactory.hpp"
#include "ib/SocketConnector.hpp"


using ib::internal::AsioEClientSocket;
using ib::internal::EWrapperFactory;

namespace IBAPI {

class SocketConnectorImpl {

 public:
  SocketConnectorImpl(Application& app, int timeout) :
      app_(app),
      timeoutSeconds_(timeout),
      running_(false),
      socketConnector_(NULL)
  {
  }

  ~SocketConnectorImpl()
  {
    if (socket_ != NULL) {
      socket_->eDisconnect();
      for (int i = 0; i < timeoutSeconds_ && socket_->isConnected(); ++i) {
        sleep(1);
      }
    }
  }

  /// @overload
  int connect(const string& host,
              unsigned int port,
              unsigned int clientId,
              SocketConnector::Strategy* strategy)
  {
    // Check on the state.
    if (socket_ && socket_->isConnected()) {
        LOG(WARNING) << "Calling eConnect on already live connection."
                     << std::endl;
        return socket_->getClientId();
    }

    // First spin up a thread that can handle the inbound messages to gateway
    const string& zmqSocketAddress = getZmqSocketAddress(host, port, clientId);
    thread_ = boost::shared_ptr<boost::thread>(
        new boost::thread(boost::bind(
            &SocketConnectorImpl::zmqHandler, this, zmqSocketAddress)));
    LOG(INFO) << "Connecting..." << std::endl;

    EWrapperFactory::ZmqAddress fromGatewayZmqAddress = "tcp://*:5555";

    EWrapper* ew =
        EWrapperFactory::getInstance()->getImpl(app_,
                                                fromGatewayZmqAddress,
                                                clientId);
    assert (ew != NULL);


    // Start a new socket.
    boost::lock_guard<boost::mutex> lock(mutex_);

    socket_ = boost::shared_ptr<AsioEClientSocket>(
        new AsioEClientSocket(ioService_, *ew));

    socket_->eConnect(host.c_str(), port, clientId);

    // Spin until connected.
    int64 limit = timeoutSeconds_ * 1000000;
    int64 start = now_micros();
    while (!socket_->isConnected() && now_micros() - start < limit) {}

    int64 elapsed = now_micros() - start;
    LOG(INFO) << "Connected in " << elapsed << " microseconds." << std::endl;

    int result;
    if (socket_->isConnected()) {
      strategy->onConnect(*socketConnector_, clientId);
      result = clientId;
      running_ = true;
    } else {
      strategy->onTimeout(*socketConnector_);
      result = -1;
      running_ = false;
    }

    // Notify any waiting thread that a connection has been made.
    socketConnected_.notify_one();
    return result;
  }

 private:

  /// Returns the address string for ZMQ -- for outbound messages to the
  /// gateway.
  std::string getZmqSocketAddress(const std::string& host,
                                         unsigned int port,
                                         int clientId)
  {
    std::ostringstream address;
    address << "inproc://" << clientId << "@" << host << ":" << port;
    std::string addr = address.str();
    return addr;
  }

  /// Handles the incoming messages from the message queue.  These
  /// messages are to be dispatched via the AsioEClientSocket to the gateway.
  void zmqHandler(const std::string& zmqSocketAddress)
  {
    // First bind to the zmq address:
    zmq::context_t zmqContext(1);
    zmq::socket_t zmqSocket(zmqContext, ZMQ_REP);
    zmqSocket.bind(zmqSocketAddress.c_str());

    LOG(INFO) << "ZMQ - SocketConnector bound to " << zmqSocketAddress
              << std::endl;

    // Wait for the socket to be connected
    int64 start = now_micros();
    boost::unique_lock<boost::mutex> lock(mutex_);

    while (!socket_ || !socket_->isConnected()) {
      socketConnected_.wait(lock);
    }

    int64 elapsed = now_micros() - start;
    VLOG(VLOG_LEVEL_IBAPI_SOCKET_CONNECTOR) << "Connected to gateway in "
                                            << elapsed << " microseconds."
                                            << std::endl;

    // Here we are connected. So now go into loop to process inbound
    // messages and forward them to the gateway.
    while (running_) {
      zmq::message_t request;

      zmqSocket.recv(&request);

      // Unpack the message and call the eclient socket:

      // Send the reply with status
      zmq::message_t reply(2);
      memcpy((void*)reply.data(), "OK", 2);
      zmqSocket.send(reply);
    }
  }

 private:
  Application& app_;
  int timeoutSeconds_;
  boost::asio::io_service ioService_; // Dedicated per connector
  boost::shared_ptr<AsioEClientSocket> socket_;
  boost::shared_ptr<boost::thread> thread_;
  boost::mutex mutex_;
  boost::condition_variable socketConnected_;
  bool running_;

 protected:
  SocketConnector* socketConnector_;
};



class SocketConnector::implementation : public SocketConnectorImpl {
 public:
  implementation(Application& app, int timeout) :
      SocketConnectorImpl(app, timeout) {}

  ~implementation() {}

  friend class SocketConnector;
};



SocketConnector::SocketConnector(Application& app, int timeout)
    : impl_(new SocketConnector::implementation(app, timeout))
{
    VLOG(VLOG_LEVEL_IBAPI_SOCKET_CONNECTOR) << "SocketConnector starting."
                                            << std::endl;
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

