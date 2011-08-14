#ifndef IBAPI_INITIATOR_H_
#define IBAPI_INITIATOR_H_

#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>

#include <Shared/EWrapper.h>
#include "ib/Application.hpp"
#include "ib/Exceptions.hpp"
#include "ib/SocketConnector.hpp"



namespace IBAPI {

class SessionSetting {
 public:
  SessionSetting(const std::string& host, unsigned int port, unsigned int id)
      : host_(host), port_(port), id_(id) {}

  const std::string& getHost() { return host_; }
  const unsigned int getPort() { return port_; }
  const unsigned int getConnectionId() { return id_; }

 private:
  const std::string& host_;
  unsigned int port_;
  unsigned int id_;
};



/**
 * Models after Initiator in QuickFIX API:
 * https://github.com/lab616/third_party/blob/master/quickfix-1.13.3/src/C++/Initiator.h
 */
class Initiator {

 public:

  Initiator(Application& app, SessionSetting& setting) :
      application_(app), sessionSetting_(setting) {};
  
  ~Initiator() {};

  virtual void start() throw ( ConfigError, RuntimeError ) {};
  virtual void block() throw ( ConfigError, RuntimeError ) {};

  virtual void stop(double timeout) {};
  virtual void stop(bool force = false) {};
 
  virtual bool isLoggedOn() { return false; };

 protected:
  Application& application_;
  SessionSetting& sessionSetting_;
};

} // namespace IBAPI

#endif // IBAPI_INITIATOR_H_
