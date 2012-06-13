#ifndef IBAPI_PROTOBUFF_MESSAGE_H_
#define IBAPI_PROTOBUFF_MESSAGE_H_

#include "log_levels.h"
#include "utils.hpp"

#include "ib/ZmqMessage.hpp"
#include "zmq/ZmqUtils.hpp"



namespace ib {
namespace internal {

template <typename P>
class ProtoBufferMessage : public ZmqMessage, public ZmqSendable
{
 public:

  ProtoBufferMessage() : ZmqMessage(), ZmqSendable()
  { }

  ProtoBufferMessage(const P& p) : ZmqMessage(), ZmqSendable(), proto_(p)
  { }

  ~ProtoBufferMessage() {}

  virtual const std::string& key() const
  {
    return proto_.key();
  }

  virtual P& proto()
  {
    return proto_;
  }

  virtual bool validate()
  {
    return proto_.IsInitialized();
  }

  virtual bool callApi(EClientPtr eclient)
  {
    proto_.set_timestamp(timestamp_);
    return callApi(proto_, eclient);
  }

  virtual size_t send(zmq::socket_t& socket, MessageId messageId)
  {
    // Copy the proto to another proto and then commit the changes
    P copy;
    copy.CopyFrom(proto_);
    copy.set_message_id(messageId);
    copy.set_timestamp(timestamp_);

    std::string buff;
    if (!copy.SerializeToString(&buff)) {
      return 0;
    }

    size_t sent = 0;
    try {

      sent += atp::zmq::send_copy(socket, key(), true);
      sent += atp::zmq::send_copy(socket, buff, false);

      messageId_ = messageId;
      proto_.CopyFrom(copy);

    } catch (zmq::error_t e) {
      API_MESSAGES_ERROR << "Error sending: " << e.what();
    }
    return sent;
  }

  virtual bool receive(zmq::socket_t& socket)
  {
    std::string frame1;
    bool more = atp::zmq::receive(socket, &frame1);
    if (more) {
      // Something wrong -- we are supposed to read only one
      // frame and all of protobuffer's data is in it.
      API_MESSAGE_BASE_LOGGER << "More data than expected: "
                              << proto_.key() << ":" << frame1;
      return false;
    } else {
      P p;
      p.ParseFromString(frame1);
      if (p.IsInitialized()) {
        proto_.CopyFrom(p);
        messageId_ = proto_.message_id();
        return true;
      }
    }
    return false;
  }

 protected:
  virtual bool callApi(const P& proto, EClientPtr eclient) = 0;

 private:
  P proto_;
  MessageId messageId_;
};

} // internal
} // ib


#endif // IBAPI_PROTOBUFF_MESSAGE_H_
