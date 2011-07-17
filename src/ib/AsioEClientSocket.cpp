
#include <sstream>
#include <glog/logging.h>

#include "ib/AsioEClientSocket.hpp"

namespace ib {
namespace internal {

AsioEClientSocket::AsioEClientSocket(boost::asio::io_service& ioService,
                                     EWrapper *ptr) :
    EClientSocketBase(ptr),
    ioService_(ioService),
    socket_(ioService),
    clientId_(0)
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
  } catch (boost::system::system_error e) {
    LOG(WARNING) << "Exception while connecting: " << e.what() << std::endl;
  }
  return ok;
}

void AsioEClientSocket::eDisconnect() {

}

int AsioEClientSocket::send(const char* buf, size_t sz) {
  return 0;
}

int AsioEClientSocket::receive(char* buf, size_t sz) {
  return 0;
}


} // internal
} // ib

