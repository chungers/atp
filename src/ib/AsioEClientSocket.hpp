#ifndef IB_ASIO_ECLIENT_SOCKET_H_
#define IB_ASIO_ECLIENT_SOCKET_H_

#include <boost/asio.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include "ib/internal.hpp"

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
class AsioEClientSocket : public IBClient {

 public:

  class EventCallback
  {
   public:
    virtual void onEventThreadStart() = 0;
    virtual void onEventThreadStop() = 0;
    virtual void onSocketConnect(bool success) = 0;
    virtual void onSocketClose(bool success) = 0;
  };

  explicit AsioEClientSocket(boost::asio::io_service& ioService,
                             EWrapper& wrapper,
                             EventCallback* callback = NULL);

  ~AsioEClientSocket();

  /// @overload EClientSocketBase
  bool eConnect(const char *host, unsigned int port, int clientId=0);

  /// Returns clientId
  int getClientId();

  /// @overload EClientSocketBase
  void eDisconnect();

  /// Event loop that checks messages on the socket
  void block();

  void reset();

 private:

  /// @overload EClientSocketBase
  bool isSocketOK() const;

  /// @overload EClientSocketBase
  int send(const char* buf, size_t sz);

  /// @overload EClientSocketBase
  int receive(char* buf, size_t sz);

  bool closeSocket();

  boost::asio::io_service& ioService_;
  tcp::socket socket_;
  boost::mutex socketMutex_;

  // No ownership of this object.
  EventCallback* callback_;

  volatile bool socketOk_;

  enum State {  STARTING, RUNNING, STOPPING, STOPPED };
  volatile State state_;

  boost::shared_ptr<boost::thread> thread_;
  boost::mutex mutex_;

  boost::condition_variable socketRunning_;
  int clientId_;

  int64 sendDt_;
  int64 receiveDt_;
  int64 processMessageDt_;
};

} // namespace internal
} // namespace ib


#endif //IB_ASIO_ECLIENT_SOCKET_H_
