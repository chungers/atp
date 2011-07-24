
#include <stdio.h>
#include <sstream>
#include <glog/logging.h>

#include <boost/bind.hpp>
#include "ib/AsioEClientSocket.hpp"

namespace ib {
namespace internal {

AsioEClientSocket::AsioEClientSocket(boost::asio::io_service& ioService,
                                     EWrapper *ptr) :
    EClientSocketBase(ptr),
    ioService_(ioService),
    socket_(ioService),
    bytesRead_(0),
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
  bool ok = false;
  try {
    socket_.connect(endpoint);
    ok = true;
    socketOk_ = true;

    // Sends client version to server
    onConnectBase();

    //LOG(INFO) << "Blocking receive for connection ACK." << std::endl;


    while (isSocketOK()) {
      checkMessages();
      }
    
    //LOG(INFO) << "Received " << bytesRead_ << " bytes. " << std::endl;

    // Schedule async read handler for incoming packets.
    /*
    socket_.async_receive(boost::asio::buffer(data_, sizeof(data_)), 0,
                          boost::bind(&AsioEClientSocket::handle_event, this,
                                      boost::asio::placeholders::error,
                                      boost::asio::placeholders::bytes_transferred));

    LOG(INFO) << "Scheduled event handler." << std::endl;
    */
    
  } catch (boost::system::system_error e) {
    LOG(WARNING) << "Exception while connecting: " << e.what() << std::endl;
  }
  return ok;
}

bool AsioEClientSocket::isSocketOK() const {
  return socketOk_;
}

void AsioEClientSocket::handle_event(const boost::system::error_code& error,
                                     size_t bytes_read) {
  LOG(INFO) << "Read " << bytes_read << " bytes: " << std::string(data_) <<  std::endl;
  bytesRead_ = bytes_read;

  while (isSocketOK()) {
    checkMessages();
  }
  /*
  socket_.async_receive(boost::asio::buffer(data_), 0,
                        boost::bind(&AsioEClientSocket::handle_event, this,
                                    boost::asio::placeholders::error,
                                    boost::asio::placeholders::bytes_transferred));
  */
}


void AsioEClientSocket::eDisconnect() {

}

int AsioEClientSocket::send(const char* buf, size_t sz) {
  // Use synchronous send because the base clientImpl expects a
  // return value of the number of bytes transferred.
  try {
    LOG(INFO) << "Sending " << sz << " bytes: " << std::string(buf) << std::endl;
    size_t sent = socket_.send(boost::asio::buffer(buf, sz));

    LOG(INFO) << "Send completed " << sent << " bytes." << std::endl;
    return sent;
  } catch (boost::system::system_error e) {
    LOG(WARNING) << "Send failed: " << e.what() << std::endl;
    return 0;
  }
}


int AsioEClientSocket::receive(char* buf, size_t sz) {
  LOG(INFO) << "Receive() called with buffer size = " << sz << std::endl;
  int read = socket_.receive(boost::asio::buffer(buf, sz));
  LOG(INFO) << "Received " << read << " bytes: " << std::string(buf) << std::endl;
  return read;
}


/*
int AsioEClientSocket::receive(char* buf, size_t sz) {
  LOG(INFO) << "Receive() called with buffer size = " << sz << std::endl;
  memcpy(buf, &data_, sizeof(bytesRead_));
  LOG(INFO) << "Copied " << bytesRead_ << " bytes: " << std::string(data_) << std::endl;
  return bytesRead_;
  } */


} // internal
} // ib

