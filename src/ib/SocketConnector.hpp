#ifndef IB_POLLING_CLIENT_H_
#define IB_POLLING_CLIENT_H_

#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>

#include "ib/EventDispatcher.hpp"
#include "ib/logging_impl.hpp"

using namespace std;

#define LOG_LEVEL 1


namespace ib {
namespace internal {


// Manages a thread with which the events from the
// EPosixClientSocket is polled.
class SocketConnector {

 public:

  SocketConnector(const unsigned int connection_id);
  ~SocketConnector();

  int connect(const string& host,
              unsigned int port, EventDispatcher* event);
  void stop();
  void join();

  void received_connected();
  void received_disconnected();
  void received_heartbeat(long time);

  bool is_connected();
  void disconnect();
  void ping();

 private :

  std::string host_;
  unsigned int port_;
  unsigned int connection_id_;

  volatile bool stop_requested_;
  volatile bool connected_;
  volatile bool pending_heartbeat_;
  volatile time_t heartbeat_deadline_;

  boost::shared_ptr<boost::thread> polling_thread_;
  boost::scoped_ptr<EPosixClientSocket> client_socket_;
  boost::scoped_ptr<EventDispatcher> dispatcher_;
  boost::mutex mutex_;

  int disconnects_;

  void connect_retry_loop();

  bool poll_socket(timeval tval);



};

} // namespace internal
} // namespace ib


#endif // IB_POLLING_CLIENT_H_

