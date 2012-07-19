#ifndef ATP_ZMQ_REACTOR_H_
#define ATP_ZMQ_REACTOR_H_

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <zmq.hpp>

#include "common.hpp"

namespace atp {
namespace zmq {


/// By definition, a reactor is required to send a reply to the
/// client on receiving the message.  This uses the ZMQ_REP / ZMQ_REQ pair.
class Reactor
{
 public:

  struct Strategy : NoCopyAndAssign {

    virtual bool respond(::zmq::socket_t& socket) { return false; }

  };


  Reactor(const int socket_type,
          const std::string& addr,
          Strategy& strategy,
          ::zmq::context_t* context = NULL);
  ~Reactor();

  const std::string& addr();

  void block();

 protected:

  /// Processes the messages from the socket.
  void process();

 private:
  const int socket_type_;
  const std::string& addr_;
  boost::shared_ptr<boost::thread> thread_;
  Strategy& strategy_;
  ::zmq::context_t* context_;
  bool ready_;
  boost::mutex mutex_;
  boost::condition_variable isReady_;
};


} // namespace zmq
} // namespace atp

#endif //ATP_ZMQ_REACTOR_H_
