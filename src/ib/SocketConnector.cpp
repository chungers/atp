
#include <sys/select.h>
#include <sys/time.h>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>

#include "ib/SocketConnector.hpp"


using namespace ib::internal;
using namespace std;

namespace ib {
namespace internal {

const int VLOG_LEVEL = 2;

DEFINE_int32(retry_sleep_seconds, 10,
             "Sleep time in seconds before retry connection.");
DEFINE_int32(max_attempts, 50,
             "Max number of connection attempts.");
DEFINE_int32(heartbeat_deadline, 5,
             "Heartbeat deadline in seconds.");
DEFINE_int32(heartbeat_interval, 10 * 60 * 60,
             "Heartbeat interval in seconds.");


///////////////////////////////////////////////////////////////
// Polling client that polls the socket in a dedicated thread.
//
SocketConnector::SocketConnector(const unsigned int connection_id)
    : connection_id_(connection_id - 1)
    , stop_requested_(false)
    , connected_(false)
    , pending_heartbeat_(false)
    , heartbeat_deadline_(0)
    , disconnects_(0)
{
}

SocketConnector::~SocketConnector()
{
}


int SocketConnector::connect(const string& host,
                             unsigned int port,
                             Strategy* strategy)
{
  host_ = host;
  port_ = port;
  strategy_.reset(strategy);

  assert(!polling_thread_);
  VLOG(LOG_LEVEL) << "Starting thread.";

  polling_thread_ = boost::shared_ptr<boost::thread>(new boost::thread(
      boost::bind(&SocketConnector::connect_retry_loop, this)));

  return 0;
}

void SocketConnector::stop()
{
  assert(polling_thread_);
  stop_requested_ = true;
  polling_thread_->join();
}

void SocketConnector::join()
{
  polling_thread_->join();
}

void SocketConnector::received_connected()
{
  boost::unique_lock<boost::mutex> lock(mutex_);
  connected_ = true;
}

void SocketConnector::received_disconnected()
{
  boost::unique_lock<boost::mutex> lock(mutex_);
  connected_ = false;
}

void SocketConnector::received_heartbeat(long time)
{
  time_t t = (time_t)time;
  struct tm * timeinfo = localtime (&t);
  VLOG(VLOG_LEVEL + 2) << "The current date/time is: " << asctime(timeinfo);

  // Lock then update the next heartbeat deadline. In case it's called
  // from another thread to message this object that heartbeat was received.
  boost::unique_lock<boost::mutex> lock(mutex_);
  heartbeat_deadline_ = 0;
  pending_heartbeat_ = false;
}


bool SocketConnector::is_connected()
{
  return (client_socket_.get())? client_socket_->isConnected() : false;
}

void SocketConnector::disconnect()
{
  if (client_socket_.get()) {
    client_socket_->eDisconnect();
    disconnects_++;
    received_disconnected();
  }
}

void SocketConnector::ping()
{
  if (client_socket_.get()) {
    client_socket_->reqCurrentTime();
  }
}

/**
 * See http://www.gnu.org/s/libc/manual/html_node/Waiting-for-I_002fO.html
 * This uses select with the timeout determined by the caller.
 * @implements EPosixClientSocketAccess
 */
bool SocketConnector::poll_socket(timeval tval)
{
  if (!client_socket_.get()) return true;
  fd_set readSet, writeSet, errorSet;

  if(client_socket_->fd() >= 0 ) {
    FD_ZERO(&readSet);
    errorSet = writeSet = readSet;

    FD_SET(client_socket_->fd(), &readSet);

    if(!client_socket_->isOutBufferEmpty())
      FD_SET(client_socket_->fd(), &writeSet);

    FD_CLR(client_socket_->fd(), &errorSet);

    int ret = select(client_socket_->fd() + 1,
                     &readSet, &writeSet, &errorSet, &tval);

    if(ret == 0) return true; // expired

    if(ret < 0) {
      // error
      VLOG(LOG_LEVEL) << "Error. Disconnecting.";
      disconnect();
      return false; // Do not continue.
    }

    if(client_socket_->fd() < 0) return false;

    if(FD_ISSET(client_socket_->fd(), &errorSet)) {
      // error on socket
      client_socket_->onError();
    }

    if(client_socket_->fd() < 0) return false;

    if(FD_ISSET(client_socket_->fd(), &writeSet)) {
      // socket is ready for writing
      client_socket_->onSend();
    }

    if(client_socket_->fd() < 0) return false;

    if(FD_ISSET(client_socket_->fd(), &readSet)) {
      // socket is ready for reading
      client_socket_->onReceive();
    }
  }
  return true;  // Ok to continue.
}


void SocketConnector::connect_retry_loop()
{
  int tries = 0;
  while (!stop_requested_) {

    connection_id_++;

    // Deletes any previously allocated resource.
    EWrapper* ewrapperImpl = strategy_.get()->getEWrapperImpl();
    client_socket_.reset(new LoggingEClientSocket(
        connection_id_, ewrapperImpl));

    LOG(INFO) << "Connecting to "
              << host_ << ":" << port_ << " @ " << connection_id_;

    client_socket_->eConnect(host_.c_str(), port_, connection_id_);

    time_t next_heartbeat = time(NULL);

    while (is_connected()) {
      struct timeval tval;
      tval.tv_usec = 0;
      tval.tv_sec = 0;

      time_t now = time(NULL);
      if (connected_ && now >= next_heartbeat) {
        // Do heartbeat
        boost::unique_lock<boost::mutex> lock(mutex_);
        ping();
        heartbeat_deadline_ = now + FLAGS_heartbeat_deadline;
        next_heartbeat = now + FLAGS_heartbeat_interval;
        pending_heartbeat_ = true;
        lock.unlock();
      }

      // Check for heartbeat timeouts
      if (pending_heartbeat_ && now > heartbeat_deadline_) {
        LOG(WARNING) << "No heartbeat in " << FLAGS_heartbeat_deadline
                     << " seconds.";
        disconnect();
        LOG(WARNING) << "Disconnected because of no heartbeat.";
        break;
      }

      // Update the select() call timeout if we have a heartbeat coming.
      // The timeout is set to the next expected heartbeat.  If the timeout
      // value is too small (or == 0), there will be excessive polling because
      // the poll_socket() will return right away.  This is usually observed
      // with a spike in cpu %.
      now = time(NULL);
      if (pending_heartbeat_) {
        tval.tv_sec = heartbeat_deadline_ - now;
      } else {
        tval.tv_sec = FLAGS_heartbeat_interval;
      }
      VLOG(VLOG_LEVEL+10) << "select() timeout=" << tval.tv_sec;

      if (!poll_socket(tval)) {
        VLOG(LOG_LEVEL) << "Error on socket. Try later.";
        break;
      }
    }

    // Push the heartbeat deadline into the future since right now we
    // are not connected.
    boost::unique_lock<boost::mutex> lock(mutex_);
    next_heartbeat = 0;
    pending_heartbeat_ = false;
    lock.unlock();

    if (tries++ >= FLAGS_max_attempts) {
      LOG(WARNING) << "Retry attempts exceeded: " << tries << ". Exiting.";
      break;
    }
    LOG(INFO) << "Sleeping " << FLAGS_retry_sleep_seconds
              << " sec. before reconnect.";
    sleep(FLAGS_retry_sleep_seconds);
    VLOG(LOG_LEVEL) << "Reconnecting.";
  }
  VLOG(LOG_LEVEL) << "Stopped.";
}


} // namespace internal
} // namespace ib
