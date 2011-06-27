#ifndef IB_POLLING_CLIENT_H_
#define IB_POLLING_CLIENT_H_

#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>

#include "ib/logging_impl.hpp"

using namespace std;

#define LOG_LEVEL 1


namespace ib {
namespace internal {


// Manages a thread with which the events from the
// EPosixClientSocket is polled.
class SocketConnector {

 public:
  class Strategy {
   public:
    virtual ~Strategy() = 0;
    virtual void onConnect() = 0;
    virtual void onError(const unsigned int errorCode) = 0;
    virtual void onHeartBeat(const long time) = 0;

    // similar to the onData() method in QuickFIX's SocketConnector::Strategy
    virtual EWrapper* getEWrapperImpl() = 0;
  };

 public:

  SocketConnector(const unsigned int connection_id);
  ~SocketConnector();

  int connect(const string& host,
              unsigned int port, Strategy* s);
  void stop();
  void join();

  void updateHeartbeat(long time);

  bool is_connected();
  void disconnect();
  void ping();

 private :

  std::string host_;
  unsigned int port_;
  unsigned int connection_id_;

  enum ConnectorState {
    RUNNING,
    DISCONNECTING,
    DISCONNECTED,
    STOPPING,
    STOPPED };

  volatile ConnectorState state_;
  volatile bool pending_heartbeat_;
  volatile time_t heartbeat_deadline_;

  boost::shared_ptr<boost::thread> polling_thread_;
  boost::scoped_ptr<EPosixClientSocket> client_socket_;
  boost::scoped_ptr<Strategy> strategy_;
  boost::mutex mutex_;

  int disconnects_;

  void connect_retry_loop();

  bool poll_socket(timeval tval);



};

} // namespace internal
} // namespace ib


#endif // IB_POLLING_CLIENT_H_

