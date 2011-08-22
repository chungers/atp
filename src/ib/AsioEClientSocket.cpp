
//#include <stdio.h>
//#include <sstream>
#include <glog/logging.h>

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/thread.hpp>

#include <EWrapper.h>
#include <TwsSocketClientErrors.h>

#include "common.hpp"
#include "ib/AsioEClientSocket.hpp"


using boost::asio::ip::tcp;

DEFINE_bool(startThread, false, "True to start thread.");

namespace ib {
namespace internal {


AsioEClientSocket::AsioEClientSocket(boost::asio::io_service& ioService,
                                     EWrapper& wrapper, bool runThread) :
    EClientSocketBase(&wrapper),
    ioService_(ioService),
    socket_(ioService),
    socketOk_(false),
    state_(STARTING),
    runThread_(runThread),
    clientId_(-1)
{
}

int AsioEClientSocket::getClientId()
{
  return clientId_;
}

/**
   Connects to the gateway.
*/
bool AsioEClientSocket::eConnect(const char *host, unsigned int port, int clientId) {
  setClientId(clientId);
  clientId_ = clientId;
  
  tcp::endpoint endpoint(boost::asio::ip::address::from_string(host), port);

  // Connect synchronously
  try {

    VLOG(VLOG_LEVEL_ASIO_ECLIENT_SOCKET) << "Connecting to " << endpoint << std::endl;

    int64 start = now_micros();
    socket_.connect(endpoint);
    int64 elapsed = now_micros() - start;
    
    socketOk_ = true;

    LOG(INFO) << "Socket connected to " << endpoint
              << " in " << elapsed << " microseconds." << std::endl;
    
    // Sends client version to server
    onConnectBase();

    // Schedule async read handler for incoming packets.
    if (runThread_) {
      assert(!thread_);
      thread_ = boost::shared_ptr<boost::thread>(
          new boost::thread(boost::bind(&AsioEClientSocket::block, this)));

      VLOG(VLOG_LEVEL_ASIO_ECLIENT_SOCKET_DEBUG)
          << "Started event thread." << std::endl;
    }

    state_ = RUNNING;
    
  } catch (boost::system::system_error e) {
    LOG(WARNING) << "Exception while connecting: " << e.what() << std::endl;
    socketOk_ = false;
  }
  return socketOk_ && state_ == RUNNING;
}


/**
   Disconnects the client from the gateway.
 */
void AsioEClientSocket::eDisconnect() {
  eDisconnectBase();
  boost::system::error_code ec;

  // Wait for the event thread to stop
  LOG(INFO) << "Stopping..." << std::endl;
  state_ = STOPPING;

  // Shut down first before closing
  socket_.shutdown(tcp::socket::shutdown_both, ec);
  if (ec) {
    LOG(WARNING) << "Failed to shutdown socket." << std::endl;
  } else {
    // Now close the socket.
    socket_.close(ec);
    if (ec) {
      LOG(WARNING) << "Failed to close socket connection." << std::endl;
    } else {
      LOG(INFO) << "Socket closed." << std::endl;
      if (thread_) {
        thread_->join();
      }
      state_ = STOPPED;
    }
  }
}

bool AsioEClientSocket::isSocketOK() const {
  return socketOk_ && state_ == RUNNING;
}


int AsioEClientSocket::send(const char* buf, size_t sz) {
  // Use synchronous send because the base clientImpl expects a
  // return value of the number of bytes transferred.
  size_t sent = 0;
  try {
    
    int64 start = now_micros();
    sent = socket_.send(boost::asio::buffer(buf, sz));
    int64 elapsed = now_micros() - start;

    VLOG(VLOG_LEVEL_ASIO_ECLIENT_SOCKET_DEBUG)
        << "Sent " << sz << " bytes in " << elapsed << " microseconds: "
        << std::string(buf)
        << std::endl;

  } catch (boost::system::system_error e) {

    if (state_ == RUNNING) {
      LOG(WARNING) << "Send failed: " << e.what() << std::endl;
    }

    socketOk_ = false;
    state_ = STOPPING;
  }

  return sent;
}


int AsioEClientSocket::receive(char* buf, size_t sz) {
  size_t read = -1;
  try {

    read = socket_.receive(boost::asio::buffer(buf, sz));

    VLOG(VLOG_LEVEL_ASIO_ECLIENT_SOCKET_DEBUG)
        << "Received " << read << " bytes: " << std::string(buf) << std::endl;

  } catch (boost::system::system_error e) {

    if (state_ == RUNNING) {
      LOG(WARNING) << "Receive failed: " << e.what() << std::endl;
    }
    socketOk_ = false;
    state_ = STOPPING;
  }

  return read;
}


// Event handling loop.  This runs in a separate thread.
void AsioEClientSocket::block() {
  LOG(INFO) << "Starts processing incoming messages." << std::endl;
  
  bool processed = true;
  while (isSocketOK() && processed) {
    try {
      int64 start = now_micros();
      processed = checkMessages();
      int64 elapsed = now_micros() - start;

      VLOG(VLOG_LEVEL_ASIO_ECLIENT_SOCKET_DEBUG)
          << "Processed message in " << elapsed << " microseconds." << std::endl;
      
    } catch (...) {
      // Implementation taken from http://goo.gl/aiOKm
      getWrapper()->error(NO_VALID_ID, CONNECT_FAIL.code(), CONNECT_FAIL.msg());
      processed = false;
      LOG(WARNING) << "Exception while processing event." << std::endl;
      state_ = STOPPING;
    }
  }
  VLOG(VLOG_LEVEL_ASIO_ECLIENT_SOCKET_DEBUG) << "Event loop terminated." << std::endl;
}

} // internal
} // ib

