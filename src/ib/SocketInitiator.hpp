#ifndef IB_INITIATOR_H_
#define IB_INITIATOR_H_

#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>

#include <Shared/EWrapper.h>
#include "ib/Application.hpp"
#include "ib/SocketConnector.hpp"
using ib::internal::SocketConnector;

namespace ib {

class SessionSetting {
 public:
  SessionSetting(const string& host, unsigned int port, unsigned int id)
      : host_(host), port_(port), id_(id) {}

 private:
  const string& host_;
  unsigned int port_;
  unsigned int id_;
};

// Similar to the QuickFIX SocketInitiator class
class SocketInitiator : SocketConnector::Strategy {

 public:

  SocketInitiator(Application& app, SessionSetting& setting);
  ~SocketInitiator();

  void start();
  void stop(double timeout);
  void stop(bool force = false);
  void block();

  void onConnect();
  void onError(const unsigned int errorCode);
  void onHeartBeat(const long time);
  EWrapper* getEWrapperImpl();


 private:
  Application& application_;
  SessionSetting& setting_;
  boost::scoped_ptr<SocketConnector> select_client_;

  volatile bool connected_;
  boost::mutex connected_mutex_;
  boost::condition_variable connected_control_;

};

}

#endif // IB_INITIATOR_H_
