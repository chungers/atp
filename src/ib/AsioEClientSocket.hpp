#ifndef IB_ASIO_ECLIENT_SOCKET_H_
#define IB_ASIO_ECLIENT_SOCKET_H_

#include <boost/asio.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>
#include <Shared/EClientSocketBase.h>

#include "common.hpp"


class EWrapper;

using boost::asio::ip::tcp;

namespace ib {
namespace internal {

/**
   EClientSocket implementation using Boost ASIO socket.
   Note that this implementation starts its own thread and calls the
   public methods available from the base class to check messages.
   It would be nice to be able to use socket.async_receive() instead; however,
   the IB socket base implementation assumes an event loop where the socket
   data is continuously polled and read.
 */
class AsioEClientSocket : public EClientSocketBase, NoCopyAndAssign {

 public:

  explicit AsioEClientSocket(boost::asio::io_service& ioService, EWrapper& ptr);
  ~AsioEClientSocket();

  /**
     @overload EClientSocketBase
  */
  bool eConnect(const char *host, unsigned int port, int clientId=0);

  /// Returns clientId
  int getClientId();
  
  /**
     @overload EClientSocketBase
   */
  void eDisconnect();
  
 private:

  /**
     @overload EClientSocketBase
  */
  bool isSocketOK() const;

  /**
     @overload EClientSocketBase
  */
  int send(const char* buf, size_t sz);

  /**
     @overload EClientSocketBase
  */
  int receive(char* buf, size_t sz);

  /**
     Event loop that checks messages on the socket
  */
  void event_loop();

  boost::asio::io_service& ioService_;
  tcp::socket socket_;

  volatile bool socketOk_;

  enum State {  STARTING, RUNNING, STOPPING, STOPPED };
  volatile State state_;
  
  boost::shared_ptr<boost::thread> eventLoopThread_;
  //boost::mutex mutext_;

  int clientId_;
};

} // namespace internal
} // namespace ib


#endif //IB_ASIO_ECLIENT_SOCKET_H_
