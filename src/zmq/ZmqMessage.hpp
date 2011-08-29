#ifndef ATP_ZMQ_MESSAGE_H_
#define ATP_ZMQ_MESSAGE_H_

#include <zmq.hpp>

namespace atp {
namespace zmq {



template <typename V>
class ZmqMessage
{

 public:
  ZmqMessage(::zmq::socket_t& socket) :  socket_(socket) {}
  ~ZmqMessage() {}

  /// Decodes the bytes from the Zmq socket and heap allocates
  /// an instance of the type T and transfers ownership to caller.
  virtual V* receive() = 0;

  /// Given a constant value of type T, encode the
  virtual bool send(const V& source) = 0;

 protected:
  template <typename T> bool frame(const T& data);
  template <typename T> bool last(const T& data);
  template <typename T> bool receive(T& output);

 private:
  ::zmq::socket_t& socket_;
};

} // zmq
} // atp



#endif // ATP_ZMQ_MESSAGE_H_
