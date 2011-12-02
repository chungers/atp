#ifndef IBAPI_SOCKET_INITIATOR_H_
#define IBAPI_SOCKET_INITIATOR_H_

#include <zmq.hpp>

#include <list>
#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>

#include "common.hpp"
#include "ib/Initiator.hpp"
#include "ib/SocketConnector.hpp"



namespace IBAPI {


/// Models after SocketInitiator in QuickFIX API:
/// http://goo.gl/S4bJa
///
/// SocketInitiator manages one or more SocketConnector.
/// Each SocketConnector has a AsioEClientSocket connection to the IB gateway.
class SocketInitiator : public Initiator,
                        public SocketConnector::Strategy {

 public:

  SocketInitiator(Application& app, std::list<SessionSetting>& settings);
  ~SocketInitiator();

  void startPublisher(const std::string& address)
      throw ( ConfigError, RuntimeError);

  /** Starts connection to gateway */
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
