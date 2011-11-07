
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


namespace ib {
namespace internal {


AsioEClientSocket::AsioEClientSocket(boost::asio::io_service& ioService,
                                     EWrapper& wrapper,
                                     AsioEClientSocket::EventCallback* cb) :
    EClientSocketBase(&wrapper),
    ioService_(ioService),
    socket_(ioService),
    callback_(cb),
    socketOk_(false),
    state_(STARTING),
    clientId_(-1)
{
  // Schedule async read handler for incoming packets.
  assert(!thread_);
  ASIO_ECLIENT_SOCKET_DEBUG << "Starting event listener thread." << std::endl;
  thread_ = boost::shared_ptr<boost::thread>(
      new boost::thread(boost::bind(&AsioEClientSocket::block, this)));
}

AsioEClientSocket::~AsioEClientSocket()
{
  ASIO_ECLIENT_SOCKET_DEBUG << "Done" << std::endl;
}

int AsioEClientSocket::getClientId()
{
  return clientId_;
}

/**
   Connects to the gateway.
*/
bool AsioEClientSocket::eConnect(const char *host,
                                 unsigned int port,
                                 int clientId)
{
  setClientId(clientId);
  clientId_ = clientId;

  tcp::endpoint endpoint(boost::asio::ip::address::from_string(host), port);

  // Connect synchronously
  try {

    ASIO_ECLIENT_SOCKET_LOGGER <<
        "Connecting to " << endpoint << std::endl;

    int64 start = now_micros();
    socket_.connect(endpoint);
    int64 elapsed = now_micros() - start;

    socketOk_ = true;

    LOG(INFO) << "Socket connected to " << endpoint
              << " (dt=" << elapsed << " microseconds)."
              << std::endl;

    // Sends client version to server
    onConnectBase();

    // Update the state and notify the waiting listener thread.
    boost::lock_guard<boost::mutex> lock(mutex_);
    state_ = RUNNING;
    socketRunning_.notify_one();

  } catch (boost::system::system_error e) {
    LOG(WARNING) << "Exception while connecting: " << e.what() << std::endl;
    socketOk_ = false;
  }
  bool result = socketOk_ && state_ == RUNNING;
  if (callback_ && socketOk_) {
    callback_->onSocketConnect(result);
  }
  return result;
}

bool AsioEClientSocket::closeSocket()
{
  boost::unique_lock<boost::mutex> lock(socketMutex_);
  if (socket_.is_open()) {
    bool success = false;
    boost::system::error_code ec;
    socket_.shutdown(tcp::socket::shutdown_both, ec);
    if (ec) {
      LOG(WARNING) << "Failed to shutdown socket: " << ec << std::endl;
    }

    // Now close the socket.
    socket_.close(ec);
    if (ec) {
      LOG(WARNING) << "Failed to close socket connection: " << ec << std::endl;
    } else {
      LOG(INFO) << "Socket closed." << std::endl;
      success = true;
    }

    if (callback_ && success) {
      callback_->onSocketClose(success);
    }
    return success;
  } else {
    return true;
  }
}

/**
   Disconnects the client from the gateway.
 */
void AsioEClientSocket::eDisconnect()
{
  eDisconnectBase();

  // Wait for the event thread to stop
  LOG(INFO) << "Stopping..." << std::endl;
  state_ = STOPPING;

  if (closeSocket() && thread_) {
    thread_->join();
    LOG(INFO) << "Listener thread stopped." << std::endl;
  }
  state_ = STOPPED;
}

bool AsioEClientSocket::isSocketOK() const
{
  return socketOk_ && state_ == RUNNING;
}


int AsioEClientSocket::send(const char* buf, size_t sz) {
  // Use synchronous send because the base clientImpl expects a
  // return value of the number of bytes transferred.
  size_t sent = 0;
  try {

    int64 start = now_micros();
    sent = socket_.send(boost::asio::buffer(buf, sz));
    sendDt_ = now_micros() - start;

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

    int64 start = now_micros();
    read = socket_.receive(boost::asio::buffer(buf, sz));
    receiveDt_ = now_micros() - start;

  } catch (boost::system::system_error e) {

    if (state_ == RUNNING) {
      LOG(WARNING) << "Receive failed: " << e.what() << std::endl;
    }
    socketOk_ = false;
    state_ = STOPPING;
    closeSocket();
  }
  return read;
}


// Event handling loop.  This runs in a separate thread.
void AsioEClientSocket::block() {

  if (callback_) {
    callback_->onEventThreadStart();
  }

  // Wait for the socket to be connected
  int64 start = now_micros();
  boost::unique_lock<boost::mutex> lock(mutex_);
  while (state_ != RUNNING) {
    socketRunning_.wait(lock);
  }

  int64 elapsed = now_micros() - start;
  LOG(INFO) << "Connection ready. Begin processing messages (dt="
            << elapsed << " microseconds)." << std::endl;

  bool processed = true;
  while (isSocketOK() && processed) {
    try {
      int64 start = now_micros();
      processed = checkMessages();
      processMessageDt_ = now_micros() - start;
    } catch (...) {
      // Implementation taken from http://goo.gl/aiOKm
      getWrapper()->error(NO_VALID_ID, CONNECT_FAIL.code(), CONNECT_FAIL.msg());
      processed = false;
      LOG(WARNING) << "Exception while processing event." << std::endl;
      state_ = STOPPING;
      closeSocket();
    }
  }
  if (callback_) {
    callback_->onEventThreadStop();
  }
  ASIO_ECLIENT_SOCKET_DEBUG << "Event thread terminated." << std::endl;
}

} // internal
} // ib

