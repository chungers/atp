#ifndef IB_ABSTRACT_SOCKET_CONNECTOR_REACTOR_H_
#define IB_ABSTRACT_SOCKET_CONNECTOR_REACTOR_H_

#include <string>

#include "zmq/Reactor.hpp"
#include "zmq/ZmqUtils.hpp"

#include "ib/ZmqMessage.hpp"


namespace ib {
namespace internal {


class AbstractSocketConnectorReactor : public atp::zmq::Reactor::Strategy
{
 public:

  struct Strategy
  {
    virtual bool IsReady() = 0;
    virtual bool IsMessageSupported(const std::string& messageKeyFrame) = 0;

    /// Constructs a typed message from the message key.  The result is
    /// optional.  @see typedef of ZmqMessagePtr
    virtual void CreateMessage(const std::string& messageKeyFrame,
                               ZmqMessagePtr& resultOptional) = 0;

    /// Tells the message to call the api
    virtual bool CallApi(ZmqMessagePtr& message) = 0;

  };

  AbstractSocketConnectorReactor(int socket_type, const std::string& address,
                                 Strategy& strategy,
                                 ::zmq::context_t* context) :
      strategy_(strategy),
      reactor_(socket_type, address, *this, context)
  {
  }

  ~AbstractSocketConnectorReactor() {}


  virtual void AfterMessage(unsigned int responseCode,
                            zmq::socket_t& socket,
                            ZmqMessagePtr& origMessageOptional) = 0;

  /// @see Reactor::Strategy
  /// This method is run from the Reactor's thread.
  virtual bool respond(zmq::socket_t& socket);

  /// Block until the reactor is done.
  void Block()
  {
    reactor_.block();
  }

 private:
  Strategy& strategy_;
  atp::zmq::Reactor reactor_;
};


class BlockingReactor : public AbstractSocketConnectorReactor
{
 public:
  BlockingReactor(const std::string& address,
                  Strategy& strategy,
                  ::zmq::context_t* context) :
      AbstractSocketConnectorReactor(ZMQ_REP, address, strategy, context)
  {
  }

  virtual void AfterMessage(unsigned int responseCode,
                            zmq::socket_t& socket,
                            ZmqMessagePtr& origMessageOptional)
  {
    atp::zmq::send_copy(socket, boost::lexical_cast<string>(responseCode));
  }

};

class NonBlockingReactor : public AbstractSocketConnectorReactor
{
 public:
  NonBlockingReactor(const std::string& address,
                     Strategy& strategy,
                     ::zmq::context_t* context) :
      AbstractSocketConnectorReactor(ZMQ_PULL, address, strategy, context)
  {
  }

  virtual void AfterMessage(unsigned int responseCode,
                            zmq::socket_t& socket,
                            ZmqMessagePtr& origMessageOptional)
  {
    // no-op, do not block
  }

};

} // internal
} // ib


#endif IB_ABSTRACT_SOCKET_CONNECTOR_REACTOR_H_
