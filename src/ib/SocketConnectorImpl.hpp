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


static const std::string& OK = "OK";

class ZmqHandler : public atp::zmq::Responder::Strategy
{
 public:

  ZmqHandler() {}
  ~ZmqHandler() {}

  bool respond(zmq::socket_t& socket)
  {
    if (eclientSocket_.get() != 0 && !eclientSocket_->isConnected()) {
      return true; // No-op -- don't read the messages from socket yet.
    }
    // Now we are connected.  Process the received messages.
    std::string msg;
    int more = atp::zmq::receive(socket, &msg);

    // just echo back
    try {
      size_t sent = atp::zmq::send_zero_copy(socket, msg);
      return sent > 0;
    } catch (zmq::error_t e) {
      LOG(ERROR) << "Exception " << e.what() << std::endl;
      return false;
    }

    return true;
  }

  void setSink(boost::shared_ptr<AsioEClientSocket>& eclientSocket)
  {
    eclientSocket_ = eclientSocket;
  }

 private:
  boost::shared_ptr<AsioEClientSocket> eclientSocket_;
};



class SocketConnectorImpl {

 public:
  SocketConnectorImpl(Application& app, int timeout,
                      const string& bindAddress) :
      app_(app),
      timeoutSeconds_(timeout),
      zmqHandler_(),
      responder_(bindAddress, zmqHandler_),
      socketConnector_(NULL)
  {
  }

  ~SocketConnectorImpl()
  {
    if (socket_.get() != 0) {
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
    if (socket_.get() != 0 && socket_->isConnected()) {
        CONNECTOR_IMPL_WARNING << "Calling eConnect on already live connection."
                               << std::endl;
        return socket_->getClientId();
    }

    EWrapperFactory::ZmqAddress publishAddress =
        atp::zmq::EndPoint::tcp(5555);

    EWrapper* ew =
        EWrapperFactory::getInstance()->getImpl(app_,
                                                publishAddress,
                                                clientId);
    assert (ew != NULL);

    // Start a new socket.
    boost::lock_guard<boost::mutex> lock(mutex_);

    if (socket_.get() == 0) {
      socket_ = boost::shared_ptr<AsioEClientSocket>(
          new AsioEClientSocket(ioService_, *ew));
    }

    socket_->eConnect(host.c_str(), port, clientId);

    // Spin until connected.
    int64 limit = timeoutSeconds_ * 1000000;
    int64 start = now_micros();
    while (!socket_->isConnected() && now_micros() - start < limit) {}

    int64 elapsed = now_micros() - start;
    CONNECTOR_IMPL_LOGGER
        << "Connected in " << elapsed << " microseconds." << std::endl;

    int result;
    if (socket_->isConnected()) {
      strategy->onConnect(*socketConnector_, clientId);
      result = clientId;
    } else {
      strategy->onTimeout(*socketConnector_);
      result = -1;
    }

    // Set up the handler
    zmqHandler_.setSink(socket_);

    return result;
  }

 private:

  Application& app_;
  int timeoutSeconds_;
  boost::asio::io_service ioService_; // Dedicated per connector
  boost::shared_ptr<AsioEClientSocket> socket_;
  boost::shared_ptr<boost::thread> thread_;
  boost::mutex mutex_;
  ZmqHandler zmqHandler_;
  atp::zmq::Responder responder_;

 protected:
  SocketConnector* socketConnector_;
};

} // namespace internal
} // namespace ib

#endif  //IB_SOCKET_CONNECTOR_IMPL_H_
