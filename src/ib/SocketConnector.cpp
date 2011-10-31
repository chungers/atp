
#include <boost/scoped_ptr.hpp>

#include "ib/AbstractSocketConnector.hpp"
#include "ib/Message.hpp"
#include "ib/ReactorStrategyFactory.hpp"

#include "zmq/ZmqUtils.hpp"


#define SOCKET_CONNECTOR_LOGGER VLOG(VLOG_LEVEL_IBAPI_SOCKET_CONNECTOR)

namespace IBAPI {

using ib::internal::AbstractSocketConnector;
using ib::internal::AsioEClientSocket;
using ib::internal::EWrapperFactory;
using ib::internal::EClientPtr;
using ib::internal::ReactorStrategyFactory;
using ib::internal::ReactorStrategy;

static bool parseMessageField(const std::string& buff, IBAPI::Message& message)
{
  // parse the message string
  size_t pos = buff.find('=');
  if (pos != std::string::npos) {
    int code = atoi(buff.substr(0, pos).c_str());
    std::string value = buff.substr(pos+1);
    LOG(INFO) << "Code: " << code
              << ", Value: " << value
              << " (" << value.length() << ")"
              ;
    switch (code) {
      case FIX::FIELD::MsgType:
      case FIX::FIELD::BeginString:
      case FIX::FIELD::SendingTime:
        message.getHeader().setField(code, value);
        break;
      case FIX::FIELD::Ext_SendingTimeMicros:
      case FIX::FIELD::Ext_OrigSendingTimeMicros:
        message.getTrailer().setField(code, value);
      default:
        message.setField(code, value);
    }
    return true;
  }
  return false;
}

class SocketConnector::implementation :
      public ib::internal::AbstractSocketConnector
{
 public:
  implementation(const std::string& zmqInboundAddr,
                 Application& app, int timeout) :
      AbstractSocketConnector(zmqInboundAddr, app, timeout),
      reactorStrategyPtr_(ReactorStrategyFactory::getInstance())
  {
  }

  ~implementation()
  {
  }

 protected:
  virtual bool handleReactorInboundMessages(
      zmq::socket_t& socket, EClientPtr eclient)
  {
    IBAPI::Message message;
    std::string buff;
    bool status = false;
    int seq = 0;
    try {
      while (1) {
        int more = atp::zmq::receive(socket, &buff);
        bool parsed = parseMessageField(buff, message);
        SOCKET_CONNECTOR_LOGGER <<
            "RECEIVE[" << seq++ << "]:" << buff << ", parsed=" << parsed;
        if (!parsed) {
          LOG(WARNING) << "Failed to parse: " << buff;
        }
        if (more == 0) break;
      }
      status = true;
    } catch (zmq::error_t e) {
      LOG(WARNING) << "Got exception: " << e.what() << std::endl;
      status = false;
    }

    // After parsing successfully, actually turn the message into a
    // EClient api call:
    if (status) {
      return reactorStrategyPtr_->handleInboundMessage(message, eclient);
    } else {
      return false;
    }
  }

  virtual zmq::socket_t* createOutboundSocket(int channel = 0)
  {
    return NULL;
  }

  friend class SocketConnector;
  boost::scoped_ptr<ReactorStrategy> reactorStrategyPtr_;
};



SocketConnector::SocketConnector(const std::string& zmqInboundAddr,
                                 Application& app, int timeout) :
    impl_(new SocketConnector::implementation(zmqInboundAddr, app, timeout))
{
  SOCKET_CONNECTOR_LOGGER << "SocketConnector started.";
  impl_->socketConnector_ = this;
}

SocketConnector::~SocketConnector()
{
}


/// QuickFIX implementation returns the socket fd.  In our case, we simply
/// return the clientId.
int SocketConnector::connect(const string& host,
                             unsigned int port,
                             unsigned int clientId,
                             Strategy* strategy)
{
  LOG(INFO) << "CONNECT" << std::endl;
  return impl_->connect(host, port, clientId, strategy);
}

} // namespace IBAPI

