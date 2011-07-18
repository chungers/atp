
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
    
    onConnectBase();

    // Schedule async read handler for incoming packets.
    socket_.async_receive(boost::asio::buffer(data_), 0,
                          boost::bind(&AsioEClientSocket::handle_event, this,
                                      boost::asio::placeholders::error,
                                      boost::asio::placeholders::bytes_transferred));
    
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
  LOG(INFO) << "Read " << bytes_read << " bytes: " << data_ <<  std::endl;
  bytesRead_ = bytes_read;
  socket_.async_receive(boost::asio::buffer(data_), 0,
                        boost::bind(&AsioEClientSocket::handle_event, this,
                                    boost::asio::placeholders::error,
                                    boost::asio::placeholders::bytes_transferred));
  
}

void AsioEClientSocket::eDisconnect() {

}

int AsioEClientSocket::send(const char* buf, size_t sz) {
  // Use synchronous send because the base clientImpl expects a
  // return value of the number of bytes transferred.
  try {
    LOG(INFO) << "Sending " << sz << " bytes: " << buf << std::endl;
    size_t sent = socket_.send(boost::asio::buffer(buf, sz));

    LOG(INFO) << "Send completed " << sent << " bytes." << std::endl;
    // Schedule async read handler for incoming packets.
    socket_.async_receive(boost::asio::buffer(data_), 0,
                          boost::bind(&AsioEClientSocket::handle_event, this,
                                      boost::asio::placeholders::error,
                                      boost::asio::placeholders::bytes_transferred));
    
    return sent;
  } catch (boost::system::system_error e) {
    LOG(WARNING) << "Send failed: " << e.what() << std::endl;
    return 0;
  }
}

int AsioEClientSocket::receive(char* buf, size_t sz) {
  LOG(INFO) << "Receive() called with buffer size = " << sz << std::endl;
  return 0;
}


} // internal
} // ib

