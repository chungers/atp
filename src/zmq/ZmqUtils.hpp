#ifndef ATP_ZMQ_UTILS_H_
#define ATP_ZMQ_UTILS_H_

#include <string>
#include <zmq.hpp>

#include "log_levels.h"

namespace atp {
namespace zmq {


template <typename T>
inline static bool frame(::zmq::socket_t & socket, const T& data, bool last) {
  ::zmq::message_t message(sizeof(T));
  memcpy(message.data(), reinterpret_cast<const void*>(&data), sizeof(T));
  bool rc = socket.send(message, last ? 0 : ZMQ_SNDMORE);
  return (rc);
}

template <typename T>
inline static bool frame(::zmq::socket_t & socket, const T& data) {
  return frame(socket, data, false);
}

template <typename T>
inline static bool last(::zmq::socket_t & socket, const T& data) {
  return frame(socket, data, true);
}

template <typename T>
inline static bool receive(::zmq::socket_t & socket, T& output) {
  ::zmq::message_t message;
  socket.recv(&message);

  // reinterpret_cast if necessary...
  memcpy(static_cast<void *>(&output), message.data(), sizeof(T));
  int64_t more;           //  Multipart detection
  size_t more_size = sizeof (more);
  socket.getsockopt(ZMQ_RCVMORE, &more, &more_size);
  VLOG(VLOG_LEVEL_ZMQ_MESSAGE)
      << '[' << output << '/' << more << ']' << std::endl;
  return more;
}


} // zmq
} // atp



#endif // ATP_ZMQ_UTILS_H_
