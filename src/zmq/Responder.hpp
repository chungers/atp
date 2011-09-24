#ifndef ATP_ZMQ_RESPONDER_H_
#define ATP_ZMQ_RESPONDER_H_

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <zmq.hpp>

namespace atp {
namespace zmq {


/// By definition, a responder is required to send a reply to the
/// client on receiving the message.  This uses the ZMQ_REP / ZMQ_REQ pair.
class Responder
{
 public:

  struct Strategy : NoCopyAndAssign {
    virtual bool respond(::zmq::socket_t& socket) = 0;
  };


  Responder(const string& addr, Strategy& strategy);
  ~Responder();

  const std::string& addr();

 protected:

  /// Processes the messages from the socket.
  void process();

 private:
  const std::string& addr_;
  boost::shared_ptr<boost::thread> thread_;
  Strategy& strategy_;
  bool ready_;
  boost::mutex mutex_;
  boost::condition_variable isReady_;
};


} // namespace zmq
} // namespace atp

#endif //ATP_ZMQ_RESPONDER_H_
