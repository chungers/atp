#ifndef IBAPI_SOCKET_INITIATOR_H_
#define IBAPI_SOCKET_INITIATOR_H_

#include <zmq.hpp>

#include <list>
#include <map>
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
                        public SocketConnector::Strategy
{

 public:

  typedef std::list< SessionSetting > SessionSettings;

  /// format:  {session_id}={gateway_ip_port}@{reactor_endpoint}
  static bool ParseSessionSettingsFromFlag(const string& flagValue,
                                           SessionSettings& settings);

  /// format:  {channel_id}={push_endpoint}
  static bool ParseOutboundChannelMapFromFlag(const string& flagValue,
                                              map<int, string>& outboundMap);

  static bool Configure(SocketInitiator& i, map<int, string>& outboundMap,
                        bool publish);


  SocketInitiator(Application& app, const SessionSettings& settings);
  ~SocketInitiator();

  /// Publish channel messages at given zmq address.  This
  /// starts an embedded publisher at the given endpoint.
  virtual void publish(int channel, const std::string& address)
      throw ( ConfigError, RuntimeError);

  /// Push channel messages to the given endpoint, without running
  /// an internal publisher.
  virtual void push(int channel, const std::string& address)
      throw ( ConfigError, RuntimeError);

  /// Starts connections to gateway
  void start() throw ( ConfigError, RuntimeError );
  void block() throw ( ConfigError, RuntimeError );
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
