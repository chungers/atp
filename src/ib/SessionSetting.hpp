#ifndef IBAPI_SESSION_SETTING_H_
#define IBAPI_SESSION_SETTING_H_

#include <iostream>

namespace IBAPI {

class SessionSetting {

 public:
  SessionSetting(unsigned int id = 0,
                 const std::string& ip = "127.0.0.1",
                 unsigned int port = 4001) :
      id_(id), ip_(ip), port_(port)
  {
  }

  /// for STL container
  SessionSetting(const SessionSetting& rhs) :
      id_(rhs.id_), ip_(rhs.ip_), port_(rhs.port_)
  {
  }

  /// for STL container
  ~SessionSetting() {}

  /// for STL container
  SessionSetting& operator=(const SessionSetting& rhs)
  {
    id_ = rhs.id_;
    ip_ = rhs.ip_;
    port_ = rhs.port_;
    return *this;
  }

  inline friend std::ostream& operator<<(std::ostream& out,
                                         const SessionSetting& rhs)
  {
    out << rhs.id_ << "@" << rhs.ip_ << ":" << rhs.port_;
    return out;
  }

  const std::string& getIp() { return ip_; }
  const unsigned int getPort() { return port_; }
  const unsigned int getConnectionId() { return id_; }

 private:
  unsigned int id_;
  std::string ip_;
  unsigned int port_;
};

} // namespace IBAPI
#endif // IBAPI_SESSION_SETTING_H_
