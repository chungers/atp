#ifndef IB_ASIO_ECLIENT_SOCKET_H_
#define IB_ASIO_ECLIENT_SOCKET_H_

#include <boost/asio.hpp>
#include <Shared/EClientSocketBase.h>

class EWrapper;

using boost::asio::ip::tcp;

namespace ib {
namespace internal {


class AsioEClientSocket : public EClientSocketBase {

 public:

  explicit AsioEClientSocket(boost::asio::io_service& ioService,
                             EWrapper *ptr);
  ~AsioEClientSocket();

  bool eConnect(const char *host, unsigned int port, int clientId=0);
  void eDisconnect();

  
 private:
  bool isSocketOK() const;
  int send(const char* buf, size_t sz);
  int receive(char* buf, size_t sz);

  void handle_event(const boost::system::error_code& error, size_t bytes_read);
  
  enum { MAX_BUFFER_LENGTH = 8 * 1024 };

  boost::asio::io_service& ioService_;
  tcp::socket socket_;
  char data_[MAX_BUFFER_LENGTH];
  size_t bytesRead_;
  int clientId_;
  bool socketOk_;
};

} // internal
} // ib



#endif //IB_ASIO_ECLIENT_SOCKET_H_
