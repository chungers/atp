#ifndef IBAPI_SOCKET_INITIATOR_H_
#define IBAPI_SOCKET_INITIATOR_H_

#include <set>
#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>

#include "common.hpp"
#include "ib/Initiator.hpp"
#include "ib/SocketConnector.hpp"



namespace IBAPI {


/// Models after SocketInitiator in QuickFIX API:
/// https://github.com/lab616/third_party/blob/master/quickfix-1.13.3/src/C++/SocketInitiator.h
///
/// SocketInitiator manages one or more SocketConnector.  Each SocketConnector has a
/// AsioEClientSocket connection to the IB gateway.
class SocketInitiator : Initiator, SocketConnector::Strategy {

 public:

  SocketInitiator(Application& app, std::set<SessionSetting>& setting);
  ~SocketInitiator();

  void start() throw ( ConfigError, RuntimeError );
  void block() throw ( ConfigError, RuntimeError );

  void stop(double timeout);
  void stop(bool force = false);
 
  bool isLoggedOn();

  /// @implement SocketConnector::Strategy
  void onConnect(SocketConnector&, int clientId);

  /// @implement SocketConnector::Strategy
  void onData(SocketConnector&, int clientId);

  /// @implement SocketConnector::Strategy
  void onDisconnect(SocketConnector&, int clientId);

  /// @implement SocketConnector::Strategy
  void onError(SocketConnector&);
  
  /// @implement SocketConnector::Strategy
  void onTimeout(SocketConnector&);

 protected:
  SocketInitiator(){}

 private:
  boost::scoped_ptr<SocketInitiator> impl_;
};

} // namespace IBAPI

#endif // IBAPI_SOCKET_INITIATOR_H_
