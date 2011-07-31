#ifndef IB_SOCKET_INITIATOR_H_
#define IB_SOCKET_INITIATOR_H_

#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>

#include <Shared/EWrapper.h>
#include "ib/Initiator.hpp"
#include "ib/SocketConnector.hpp"



namespace IBAPI {

/**
 * Models after SocketInitiator in QuickFIX API:
 * https://github.com/lab616/third_party/blob/master/quickfix-1.13.3/src/C++/SocketInitiator.h
 */
class SocketInitiator : Initiator, SocketConnector::Strategy {

 public:

  SocketInitiator(Application& app, SessionSetting& setting);
  ~SocketInitiator();

  void start() throw ( ConfigError, RuntimeError );
  void block() throw ( ConfigError, RuntimeError );

  void stop(double timeout);
  void stop(bool force = false);
 
  bool isLoggedOn();

  // Implements SocketConnector::Strategy
  void onConnect(SocketConnector&, int clientId);
  EWrapper* getEWrapperImpl();

  void onDisconnect(SocketConnector&, int clientId);
  void onError(SocketConnector&, const int clientId,
               const unsigned int errorCode);
  void onTimeout(const long time);

};

} // namespace IBAPI

#endif // IB_SOCKET_INITIATOR_H_
