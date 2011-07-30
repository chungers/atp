
#include <stdio.h>
#include <sstream>
#include <gflags/gflags.h>
#include <glog/logging.h>

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/thread.hpp>

#include "utils.hpp"
#include "ib/AsioEClientSocket.hpp"


using boost::asio::ip::tcp;

namespace ib {
namespace internal {

const int LOG_LEVEL = 10;

AsioEClientSocket::AsioEClientSocket(boost::asio::io_service& ioService,
                                     EWrapper *ptr) :
    EClientSocketBase(ptr),
    ioService_(ioService),
    socket_(ioService),
    clientId_(0),
    socketOk_(false)
{
  
}

AsioEClientSocket::~AsioEClientSocket() {

}

bool AsioEClientSocket::eConnect(const char *host, unsigned int port, int clientId) {
  clientId_ = clientId;
  tcp::endpoint endpoint(boost::asio::ip::address::from_string(host), port);

  // Connect synchronously
  try {

    VLOG(LOG_LEVEL) << "Connecting to " << endpoint << std::endl;

    int64 start = now_micros();
    
    socket_.connect(endpoint);

    int64 elapsed = now_micros() - start;
    
    socketOk_ = true;

    LOG(INFO) << "Connected in " << elapsed << " microseconds." << std::endl;
    
    // Sends client version to server
    onConnectBase();
    
    // Schedule async read handler for incoming packets.
    assert(!eventLoopThread_);

    eventLoopThread_ = boost::shared_ptr<boost::thread>(
        new boost::thread(boost::bind(&AsioEClientSocket::event_loop, this)));

    VLOG(LOG_LEVEL) << "Started event thread." << std::endl;
    
  } catch (boost::system::system_error e) {
    LOG(WARNING) << "Exception while connecting: " << e.what() << std::endl;
    socketOk_ = false;
  }
  return socketOk_;
}


void AsioEClientSocket::eDisconnect() {

}

bool AsioEClientSocket::isSocketOK() const {
  return socketOk_;
}


void AsioEClientSocket::event_loop() {
  while (isSocketOK()) {
    int64 start = now_micros();
    checkMessages();
    int64 elapsed = now_micros() - start;
  }
}


int AsioEClientSocket::send(const char* buf, size_t sz) {
  // Use synchronous send because the base clientImpl expects a
  // return value of the number of bytes transferred.
  size_t sent = 0;
  try {
    
    int64 start = now_micros();
    sent = socket_.send(boost::asio::buffer(buf, sz));
    int64 elapsed = now_micros() - start;

    VLOG(LOG_LEVEL) << "Sent " << sz << " bytes in " << elapsed << " microseconds: "
                    << std::string(buf)
                    << std::endl;

  } catch (boost::system::system_error e) {

    LOG(WARNING) << "Send failed: " << e.what() << std::endl;
    socketOk_ = false;
  }

  return sent;
}


int AsioEClientSocket::receive(char* buf, size_t sz) {
  size_t read = -1;
  try {

    read = socket_.receive(boost::asio::buffer(buf, sz));
    VLOG(LOG_LEVEL) << "Received " << read << " bytes: " << std::string(buf) << std::endl;

  } catch (boost::system::system_error e) {

    LOG(WARNING) << "Receive failed: " << e.what() << std::endl;
    socketOk_ = false;
  }

  return read;
}







} // internal
} // ib

