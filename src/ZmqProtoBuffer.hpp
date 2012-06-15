#ifndef ATP_ZMQ_PROTOBUFFER_H_
#define ATP_ZMQ_PROTOBUFFER_H_

#include "log_levels.h"
#include "utils.hpp"

#include <zmq.hpp>
#include "zmq/ZmqUtils.hpp"

using ::zmq::error_t;
using ::zmq::socket_t;

namespace atp {

template <typename P>
size_t send(socket_t& socket,
            boost::uint64_t timestamp,
            boost::uint64_t messageId,
            P& proto)
{
  // Copy the proto to another proto and then commit the changes
  P copy;
  copy.CopyFrom(proto);
  copy.set_message_id(messageId);
  copy.set_timestamp(timestamp);

  std::string buff;
  if (!copy.SerializeToString(&buff)) {
    return 0;
  }

  size_t sent = 0;
  try {

    sent += atp::zmq::send_copy(socket, proto.key(), true);
    sent += atp::zmq::send_copy(socket, buff, false);

    proto.CopyFrom(copy);

  } catch (error_t e) {
    ZMQ_PROTO_MESSAGE_ERROR << "Error sending: " << e.what();
  }
  return sent;
}


template <typename P>
bool receive(socket_t& socket, P& proto)
{
  std::string frame1;
  bool more = atp::zmq::receive(socket, &frame1);
  if (more) {
    // Something wrong -- we are supposed to read only one
    // frame and all of protobuffer's data is in it.
    ZMQ_PROTO_MESSAGE_LOGGER << "More data than expected: "
                             << proto.key() << ":" << frame1;
    return false;
  } else {
      P p;
      p.ParseFromString(frame1);
      if (p.IsInitialized()) {
        proto.CopyFrom(p);
        return true;
      }
  }
  return false;
}


} // atp



#endif //ATP_ZMQ_PROTOBUFFER_H_
