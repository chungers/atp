#ifndef IBAPI_SESSION_SETTING_H_
#define IBAPI_SESSION_SETTING_H_

#include <iostream>

namespace IBAPI {

using std::string;

class SessionSetting {

 public:
  SessionSetting(unsigned int id = 0,
                 const string& gatewayIp = "127.0.0.1",
                 unsigned int gatewayPort = 4001,
                 const string& zmqInboundAddr = "tcp://127.0.0.1:5555",
                 const string& zmqOutboundAddr = "tcp://127.0.0.1:7777"
                 ) :
      id_(id), gatewayIp_(gatewayIp), gatewayPort_(gatewayPort),
      zmqInboundAddr_(zmqInboundAddr),
      zmqOutboundAddr_(zmqOutboundAddr)
  {
  }

  /// for STL container
  SessionSetting(const SessionSetting& rhs) :
      id_(rhs.id_), gatewayIp_(rhs.gatewayIp_), gatewayPort_(rhs.gatewayPort_),
      zmqInboundAddr_(rhs.zmqInboundAddr_),
      zmqOutboundAddr_(rhs.zmqOutboundAddr_)
  {
  }

  /// for STL container
  ~SessionSetting() {}

  /// for STL container
  SessionSetting& operator=(const SessionSetting& rhs)
  {
    id_ = rhs.id_;
    gatewayIp_ = rhs.gatewayIp_;
    gatewayPort_ = rhs.gatewayPort_;
    zmqInboundAddr_ = rhs.zmqInboundAddr_;
    zmqOutboundAddr_ = rhs.zmqOutboundAddr_;
    return *this;
  }

  inline friend std::ostream& operator<<(std::ostream& out,
                                         const SessionSetting& rhs)
  {
    out << rhs.id_ << "@" << rhs.gatewayIp_ << ":" << rhs.gatewayPort_;
    out << "=>" << rhs.zmqOutboundAddr_;
    out << "||" << rhs.zmqInboundAddr_;
    return out;
  }

  const std::string& getIp()
  {
    return gatewayIp_;
  }

  const unsigned int getPort()
  {
    return gatewayPort_;
  }

  const unsigned int getConnectionId()
  {
    return id_;
  }

  const std::string& getZmqInboundAddr()
  {
    return zmqInboundAddr_;
  }

  const std::string& getZmqOutboundAddr()
  {
    return zmqOutboundAddr_;
  }

 private:
  unsigned int id_;
  std::string gatewayIp_;
  unsigned int gatewayPort_;
  std::string zmqInboundAddr_;
  std::string zmqOutboundAddr_;
};

} // namespace IBAPI
#endif // IBAPI_SESSION_SETTING_H_
