#ifndef ATP_ZMQ_UTILS_H_
#define ATP_ZMQ_UTILS_H_

#include <string>
#include <zmq.hpp>

#include "log_levels.h"

namespace atp {
namespace zmq {

void mem_free(void* mem, void* mem2)
{
  VLOG(VLOG_LEVEL_ZMQ_MEM_FREE) << "Freeing memory: " << mem
                                 << ", " << mem2 << std::endl;
}

inline static bool receive(::zmq::socket_t & socket, std::string* output) {
  ::zmq::message_t message;
  socket.recv(&message);
  output->assign(static_cast<char*>(message.data()), message.size());
  int64_t more;           //  Multipart detection
  size_t more_size = sizeof (more);
  socket.getsockopt(ZMQ_RCVMORE, &more, &more_size);
  return more;
}

/// Special zero copy send.  This uses a dummy memory free function.  Note
/// the ownership of the data buffer belongs to the input string. The input
/// string needs to be alive long enough for the network call to send to occur.
inline static int zero_copy_send(::zmq::socket_t& socket,
                                 const std::string& input,
                                 bool sendMore = false)
{
  const char* buff = input.c_str();
  size_t size = input.size();
  // Force zero copy by providing a mem free function that does
  // nothing (ownership of buffer still belongs to the input)
  ::zmq::message_t frame((void*)(buff), size, mem_free);
  int more = (sendMore) ? ZMQ_SNDMORE : 0;
  socket.send(frame, more);
  return size;
}


} // zmq
} // atp



#endif // ATP_ZMQ_UTILS_H_
