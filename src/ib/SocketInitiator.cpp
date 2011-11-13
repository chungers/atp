
#include <map>
#include <list>

#include <Shared/EWrapper.h>

#include "common.hpp"
#include "log_levels.h"
#include "ib/SocketInitiator.hpp"
#include "ib/EWrapperFactory.hpp"
#include "ib/SessionID.hpp"


using ib::internal::EWrapperFactory;

namespace IBAPI {


class SocketInitiatorImpl : public SocketInitiator {

 public :
  SocketInitiatorImpl(Application& app,
                      std::list<SessionSetting>& settings,
                      SocketConnector::Strategy& strategy) :
      application_(app),
      sessionSettings_(settings),
      strategy_(strategy)
  { }

  ~SocketInitiatorImpl() {}

  /// @overload Initiator
  void start() throw ( ConfigError, RuntimeError )
  {
    // Start up the socket connectors one by one.
    std::list<SessionSetting>::iterator itr;
    for (itr = sessionSettings_.begin();
         itr != sessionSettings_.end();
         ++itr) {

      IBAPI_SOCKET_INITIATOR_LOGGER
          << "Starting connector "
          << itr->getConnectionId()
          << ", inbound: " << itr->getZmqInboundAddr();

      SessionID sessionId = static_cast<SessionID>(itr->getConnectionId());
      boost::shared_ptr<SocketConnector> s =
          boost::shared_ptr<SocketConnector>(
              new SocketConnector(itr->getZmqInboundAddr(),
                                  application_, sessionId));
      socketConnectors_[sessionId] = s;

      IBAPI_SOCKET_INITIATOR_LOGGER << "SocketConnector: " << *itr;
      s->connect(itr->getIp(), itr->getPort(), sessionId, &strategy_);
    }
  }

  /// @overload Initiator
  void block() throw ( ConfigError, RuntimeError )
  {

  }

  /// @overload Initiator
  void stop(double timeout)
  {
    IBAPI_SOCKET_INITIATOR_LOGGER
        << "Stopping connector with timeout = "
        << timeout << std::endl;
  }

  /// @overload Initiator
  void stop(bool force) {
    IBAPI_SOCKET_INITIATOR_LOGGER
        << "Stopping connector with force = "
        << force << std::endl;
  }

  /// @overload Initiator
  bool isLoggedOn() {
    return false;
  }

 private:
  Application& application_;
  std::list<SessionSetting>& sessionSettings_;
  SocketConnector::Strategy& strategy_;
  std::map< SessionID, boost::shared_ptr<SocketConnector> > socketConnectors_;
};


SocketInitiator::SocketInitiator(Application& app,
                                 std::list<SessionSetting>& settings)
    : impl_(new SocketInitiatorImpl(app, settings, *this))
{
}

SocketInitiator::~SocketInitiator()
{
}

/// @overload Initiator
void SocketInitiator::start() throw ( ConfigError, RuntimeError )
{
  impl_->start();
}

/// @overload Initiator
void SocketInitiator::block() throw ( ConfigError, RuntimeError )
{
  impl_->block();
}

/// @overload Initiator
void SocketInitiator::stop(double timeout)
{
  impl_->stop(timeout);
}

/// @overload Initiator
void SocketInitiator::stop(bool force)
{
  impl_->stop(force);
}

/// @overload Initiator
bool SocketInitiator::isLoggedOn()
{
  return impl_->isLoggedOn();
}

/// @implement SocketConnector::Strategy
void SocketInitiator::onConnect(SocketConnector& connector, int clientId)
{
  IBAPI_SOCKET_INITIATOR_LOGGER
      << "Connection (" << clientId << ") established.";
}

/// @implement SocketConnector::Strategy
void SocketInitiator::onData(SocketConnector& connector, int clientId)
{
  IBAPI_SOCKET_INITIATOR_LOGGER
      << "Connection (" << clientId << ") data.";
}

/// @implement SocketConnector::Strategy
void SocketInitiator::onError(SocketConnector& connector)
{
  IBAPI_SOCKET_INITIATOR_LOGGER
      << "Connection error.";
}

/// @implement SocketConnector::Strategy
void SocketInitiator::onTimeout(SocketConnector& connector)
{
  IBAPI_SOCKET_INITIATOR_LOGGER
      << "Connection timeout.";
}

/// @implement SocketConnector::Strategy
void SocketInitiator::onDisconnect(SocketConnector& connector,
                                   int clientId)
{
  IBAPI_SOCKET_INITIATOR_LOGGER
      << "Connection (" << clientId << ") disconnected.";
}

} // namespace ib

