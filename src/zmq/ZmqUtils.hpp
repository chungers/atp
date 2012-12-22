#ifndef ATP_ZMQ_UTILS_H_
#define ATP_ZMQ_UTILS_H_

#include <stdio.h>
#include <string>
#include <sstream>
#include <zmq.hpp>

#include "log_levels.h"

namespace atp {
namespace zmq {

struct EndPoint {
  static std::string inproc(const std::string& name)
  {
    return "inproc://" + name;
  }

  static std::string ipc(const std::string& name)
  {
    return "ipc://" + name;
  }

  /// As of ZMQ 2.1.7, hostname must be resolvable (not '*')
  static std::string tcp(int port, const std::string& host="127.0.0.1")
  {
    std::ostringstream oss;
    oss << "tcp://" << host << ":" << port;
    return oss.str();
  }
};


static void mem_free(void* mem, void* mem2)
{
  ZMQ_MEM_FREE_DEBUG << "Freeing memory: " << mem
                     << ", " << mem2 << std::endl;
}

inline static bool receive(::zmq::socket_t & socket, std::string* output) {
  ::zmq::message_t message;
  socket.recv(&message);
  if (message.size() > 0) {
    output->assign(static_cast<char*>(message.data()), message.size());

    //  Multipart detection
#ifdef ZMQ_3X
    int more = 0;
    size_t more_size = sizeof (more);
    socket.getsockopt(ZMQ_RCVMORE, &more, &more_size);
    return more > 0;
#else
    int64_t more;
    size_t more_size = sizeof (more);
    socket.getsockopt(ZMQ_RCVMORE, &more, &more_size);
    return more;
#endif
  } else {
    return receive(socket, output);
  }
}

/// Special zero copy send.  This uses a dummy memory free function.  Note
/// the ownership of the data buffer belongs to the input string. The input
/// string needs to be alive long enough for the network call to send to occur.
inline static size_t send_zero_copy(::zmq::socket_t& socket,
                                    const std::string& input,
                                    bool sendMore = false)
{
  const char* buff = input.data();
  size_t size = input.size();
  // Force zero copy by providing a mem free function that does
  // nothing (ownership of buffer still belongs to the input)
  ::zmq::message_t frame((void*)(buff), size, mem_free);
  int more = (sendMore) ? ZMQ_SNDMORE : 0;
  socket.send(frame, more);
  return size;
}

/// Special zero copy send.  This uses a dummy memory free function.  Note
/// the ownership of the data buffer belongs to the input string. The input
/// string needs to be alive long enough for the network call to send to occur.
inline static size_t send_zero_copy(::zmq::socket_t& socket,
                                    const char* buff, size_t len,
                                    bool sendMore = false)
{
  // Force zero copy by providing a mem free function that does
  // nothing (ownership of buffer still belongs to the input)
  ::zmq::message_t frame((void*)(buff), len, mem_free);
  int more = (sendMore) ? ZMQ_SNDMORE : 0;
  socket.send(frame, more);
  return len;
}

/// Send by memcpy.  This doesn't avoid the cost of memcpy but is safer
/// because the buffer is copied to the zmq message buffer.
inline static size_t send_copy(::zmq::socket_t& socket,
                               const std::string& input,
                               bool sendMore = false)
{
  size_t size = input.size();
  ::zmq::message_t frame(size);
  memcpy(frame.data(), input.data(), size);
  int more = (sendMore) ? ZMQ_SNDMORE : 0;
  socket.send(frame, more);
  return size;
}


/// Send by memcpy.  This doesn't avoid the cost of memcpy but is safer
/// because the buffer is copied to the zmq message buffer.
template <typename T>
inline static size_t send_copy(::zmq::socket_t& socket,
                               const T input,
                               bool sendMore = false)
{
  std::ostringstream oss;
  oss << input;
  return send_copy(socket, oss.str(), sendMore);
}

/// Send by memcpy.  This doesn't avoid the cost of memcpy but is safer
/// because the buffer is copied to the zmq message buffer.
inline static size_t send_copy(::zmq::socket_t& socket,
                               const char* buff, size_t len,
                               bool sendMore = false)
{
  ::zmq::message_t frame(len);
  memcpy(frame.data(), buff, len);
  int more = (sendMore) ? ZMQ_SNDMORE : 0;
  socket.send(frame, more);
  return len;
}
} // zmq
} // atp



#endif // ATP_ZMQ_UTILS_H_
