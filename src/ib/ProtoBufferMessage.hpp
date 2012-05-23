#ifndef IBAPI_PROTOBUFF_MESSAGE_H_
#define IBAPI_PROTOBUFF_MESSAGE_H_

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
    return callApi(proto_, eclient);
  }

  virtual size_t send(zmq::socket_t& socket)
  {
    std::string buff;
    if (!proto_.SerializeToString(&buff)) {
      return 0;
    }
    size_t sent = 0;
    sent += atp::zmq::send_copy(socket, key(), true);
    sent += atp::zmq::send_copy(socket, buff, false);
    return sent;
  }

  virtual bool receive(zmq::socket_t& socket)
  {
    std::string buff;
    bool more = atp::zmq::receive(socket, &buff);
    if (more) {
      // Something wrong -- we are supposed to read only one
      // frame and all of protobuffer's data is in it.
      API_MESSAGE_BASE_LOGGER << "More data than expected: "
                              << proto_.key() << ":" << buff;
      return false;
    } else {
      P p;
      p.ParseFromString(buff);
      if (p.IsInitialized()) {
        proto_.CopyFrom(p);
      }
    }
    return proto_.IsInitialized();
  }

 protected:
  virtual bool callApi(const P& proto, EClientPtr eclient) = 0;

 private:
  P proto_;
};

} // internal
} // ib


#endif // IBAPI_PROTOBUFF_MESSAGE_H_
