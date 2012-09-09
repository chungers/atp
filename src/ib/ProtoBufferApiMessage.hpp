#ifndef IBAPI_PROTOBUFF_API_MESSAGE_H_
#define IBAPI_PROTOBUFF_API_MESSAGE_H_

#include "log_levels.h"
#include "utils.hpp"
#include "ZmqProtoBuffer.hpp"

#include "ib/ZmqMessage.hpp"
#include "zmq/ZmqUtils.hpp"



namespace ib {
namespace internal {

/// This class bridges received protobuffer to actual api calls.

/// CRTP pattern -- Curiously recurring template pattern for cloning.
template < typename P, typename Derived >
class ProtoBufferApiMessage : public ZmqMessage
{
 public:

  ProtoBufferApiMessage() : ZmqMessage()
  { }

  ProtoBufferApiMessage(P& p) : ZmqMessage(), proto_(p)
  { }

  ~ProtoBufferApiMessage() {}

  virtual const std::string key() const
  {
    return proto_.GetTypeName();
  }

  virtual P& proto()
  {
    return proto_;
  }

  virtual bool ParseFromString(const string& s)
  {
    return proto_.ParseFromString(s);
  }

  virtual bool SerializeToString(string* buff)
  {
    return proto_.SerializeToString(buff);
  }

  virtual bool validate()
  {
    return proto_.IsInitialized();
  }

  virtual ZmqMessage* clone() const
  {
    return new Derived(static_cast<const Derived&>(*this));
  }

  virtual bool callApi(EClientPtr eclient)
  {
    proto_.set_timestamp(timestamp_);
    return callApi(proto_, eclient);
  }

  virtual size_t send(zmq::socket_t& socket, MessageId messageId)
  {
    return atp::send<P>(socket, timestamp_, messageId, proto_);
  }

  virtual bool receive(zmq::socket_t& socket)
  {
    bool success = atp::receive<P>(socket, proto_);
    if (success) {
      messageId_ = proto_.message_id();
    }
    return success;
  }
 protected:
  virtual bool callApi(const P& proto, EClientPtr eclient) = 0;

 private:
  P proto_;
  MessageId messageId_;
};

} // internal
} // ib


#endif // IBAPI_PROTOBUFF_API_MESSAGE_H_
