
#include <glog/logging.h>

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/thread.hpp>

#include <EWrapper.h>
#include <TwsSocketClientErrors.h>

#include "common.hpp"
#include "varz/varz.hpp"

#include "ib/AsioEClientDriver.hpp"


using boost::asio::ip::tcp;

DEFINE_VARZ_int64(asio_socket_connection_exceptions, 0, "");
DEFINE_VARZ_int64(asio_socket_connection_resets, 0, "");
DEFINE_VARZ_int64(asio_socket_connection_closes, 0, "");
DEFINE_VARZ_int64(asio_socket_connects, 0, "");
DEFINE_VARZ_int64(asio_socket_disconnects, 0, "");

DEFINE_VARZ_int64(asio_socket_send_errors, 0, "");
DEFINE_VARZ_int64(asio_socket_send_latency_micros, 0, "");
DEFINE_VARZ_int64(asio_socket_send_latency_micros_total, 0, "");
DEFINE_VARZ_int64(asio_socket_send_latency_micros_count, 0, "");

DEFINE_VARZ_int64(asio_socket_receive_errors, 0, "");
DEFINE_VARZ_int64(asio_socket_event_loop_errors, 0, "");
DEFINE_VARZ_int64(asio_socket_event_loop_stopped, false, "");

namespace ib {
namespace internal {


AsioEClientDriver::AsioEClientDriver(boost::asio::io_service& ioService,
                                     ApiProtocolHandler& protocolHandler,
                                     EventCallback* cb) :
    ioService_(ioService),
    protocolHandler_(protocolHandler),
    socket_(ioService),
    callback_(cb),
    socketOk_(false),
    state_(STARTING),
    clientId_(-1)
{
  // Schedule async read handler for incoming packets.
  assert(!thread_);

  protocolHandler_.SetApiSocket(*this);

  ASIO_ECLIENT_SOCKET_DEBUG << "Starting event listener thread." << std::endl;
  thread_ = boost::shared_ptr<boost::thread>(
      new boost::thread(boost::bind(&AsioEClientDriver::block, this)));
}

AsioEClientDriver::~AsioEClientDriver()
{
  ASIO_ECLIENT_SOCKET_DEBUG << "Done" << std::endl;
}

int AsioEClientDriver::getClientId()
{
  return clientId_;
}

/**
   Connects to the gateway.
*/
bool AsioEClientDriver::Connect(const char *host,
                                unsigned int port,
                                int clientId)
{
  protocolHandler_.SetClientId(clientId);
  clientId_ = clientId;

  tcp::endpoint endpoint(boost::asio::ip::address::from_string(host), port);

  // Connect synchronously
  try {

    VARZ_asio_socket_connects++;

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
    protocolHandler_.OnConnect();

    // Update the state and notify the waiting listener thread.
    boost::lock_guard<boost::mutex> lock(mutex_);
    state_ = RUNNING;
    socketRunning_.notify_one();

  } catch (boost::system::system_error e) {
    LOG(WARNING) << "Exception while connecting: " << e.what() << std::endl;

    VARZ_asio_socket_connection_exceptions++;
    socketOk_ = false;
  }
  bool result = socketOk_ && state_ == RUNNING;
  if (callback_ && socketOk_) {
    callback_->onSocketConnect(result);
  }

  return result;
}

void AsioEClientDriver::reset()
{
  boost::unique_lock<boost::mutex> lock(socketMutex_);

  if (socket_.is_open()) {

    VARZ_asio_socket_connection_resets++;

    boost::system::error_code ec;
    socket_.close(ec);

    if (ec) {
      LOG(WARNING) << "Failed to close socket connection: " << ec;
    } else {
      LOG(INFO) << "Socket closed.";
    }
  }
}

bool AsioEClientDriver::closeSocket()
{
  boost::unique_lock<boost::mutex> lock(socketMutex_);

  if (socket_.is_open()) {
    bool success = false;
    boost::system::error_code ec;
    socket_.shutdown(tcp::socket::shutdown_both, ec);
    if (ec) {
      LOG(WARNING) << "Failed to shutdown socket: " << ec;
    }

    // Now close the socket.
    socket_.close(ec);
    if (ec) {
      LOG(WARNING) << "Failed to close socket connection: " << ec;
    } else {
      LOG(INFO) << "Socket closed.";
      success = true;
    }

    if (callback_ && success) {

      VARZ_asio_socket_connection_closes++;

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
void AsioEClientDriver::Disconnect()
{
  protocolHandler_.OnDisconnect();

  VARZ_asio_socket_disconnects++;

  // Wait for the event thread to stop
  LOG(INFO) << "Stopping..." << std::endl;
  state_ = STOPPING;

  if (closeSocket() && thread_) {
    thread_->join();
    LOG(INFO) << "Listener thread stopped." << std::endl;
  }
  state_ = STOPPED;
}

bool AsioEClientDriver::IsConnected() const
{
  return protocolHandler_.IsConnected();
}

bool AsioEClientDriver::IsSocketOK() const
{
  return socketOk_ && state_ == RUNNING;
}

EClientPtr AsioEClientDriver::GetEClient()
{
  return EClientPtr(&(protocolHandler_.GetEClient()));
}

/// @implements ApiSocket::Send
int AsioEClientDriver::Send(const char* buf, size_t sz) {
  // Use synchronous send because the base clientImpl expects a
  // return value of the number of bytes transferred.
  size_t sent = 0;
  try {

    int64 start = now_micros();

    sent = socket_.send(boost::asio::buffer(buf, sz));

    sendDt_ = now_micros() - start;

    VARZ_asio_socket_send_latency_micros = sendDt_;
    VARZ_asio_socket_send_latency_micros_total += sendDt_;
    VARZ_asio_socket_send_latency_micros_count++;

  } catch (boost::system::system_error e) {

    VARZ_asio_socket_send_errors++;

    if (state_ == RUNNING) {
      LOG(WARNING) << "Send failed: " << e.what() << std::endl;
    }

    socketOk_ = false;
    state_ = STOPPING;
  }

  return sent;
}


/// @implement ApiSocket::Receive
int AsioEClientDriver::Receive(char* buf, size_t sz) {
  size_t read = -1;
  try {

    read = socket_.receive(boost::asio::buffer(buf, sz));

  } catch (boost::system::system_error e) {

    VARZ_asio_socket_receive_errors++;

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
void AsioEClientDriver::block() {

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
  while (IsSocketOK() && processed) {
    try {

      processed = protocolHandler_.CheckMessages();

    } catch (...) {
      // Implementation taken from http://goo.gl/aiOKm
      VARZ_asio_socket_event_loop_errors++;

      // getWrapper()->error(NO_VALID_ID,
      //                     CONNECT_FAIL.code(),
      //                     CONNECT_FAIL.msg());
      protocolHandler_.OnError(NO_VALID_ID,
                               CONNECT_FAIL.code(),
                               CONNECT_FAIL.msg());

      processed = false;
      LOG(ERROR) << "Setting state to STOPPING and closing socket.";
      state_ = STOPPING;
      closeSocket();
    }
  }

  VARZ_asio_socket_event_loop_stopped = true;

  if (callback_) {
    callback_->onEventThreadStop();
  }
  ASIO_ECLIENT_SOCKET_DEBUG << "Event thread terminated." << std::endl;
}

} // internal
} // ib

