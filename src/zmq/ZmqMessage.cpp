
#include <string>
#include <glog/logging.h>

#include "zmq/ZmqMessage.hpp"
#include "zmq/ZmqUtils.hpp"


namespace atp {
namespace zmq {

template <typename V>
template <typename T> bool ZmqMessage<V>::frame(const T& data)
{
  return frame(socket_, data);
}

template <typename V>
template <typename T> bool ZmqMessage<V>::last(const T& data)
{
  return last(socket_, data);
}

template <typename V>
template <typename T> bool ZmqMessage<V>::receive(T& output)
{
  return receive(socket_, output);
}


} // zmq
} // atp
