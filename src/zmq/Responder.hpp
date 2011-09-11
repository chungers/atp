#ifndef ATP_ZMQ_RESPONDER_H_
#define ATP_ZMQ_RESPONDER_H_

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <zmq.hpp>

namespace atp {
namespace zmq {


struct SocketReader : NoCopyAndAssign {
  virtual bool operator()(::zmq::socket_t& socket) = 0;
};

struct SocketWriter : NoCopyAndAssign {
  virtual bool operator()(::zmq::socket_t& socket) = 0;
};

class Responder
{
 public:
  Responder(const string& addr, SocketReader& reader,
            SocketWriter& writer);
  ~Responder();

  /// The current zmq context
  ::zmq::context_t& context();

  void stop();

 protected:

  /// Processes the messages from the socket.
  void process();

 private:
  ::zmq::context_t context_;
  ::zmq::socket_t socket_;
  bool running_;
  boost::shared_ptr<boost::thread> thread_;
  SocketReader& reader_;
  SocketWriter& writer_;
  boost::mutex mutex_;
};


} // namespace zmq
} // namespace atp

#endif //ATP_ZMQ_RESPONDER_H_
