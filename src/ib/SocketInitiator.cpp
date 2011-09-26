
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


class SocketInitiatorImpl : public SocketInitiator,
                            public SocketConnector::Strategy {

 public :
  SocketInitiatorImpl(Application& app, std::list<SessionSetting>& settings) :
      application_(app),
      sessionSettings_(settings)
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

      SessionID sessionId = static_cast<SessionID>(itr->getConnectionId());
      boost::shared_ptr<SocketConnector> s = boost::shared_ptr<SocketConnector>(
          new SocketConnector(application_, sessionId));
      socketConnectors_[sessionId] = s;

      // Start the connection:
      LOG(INFO) << "Connecting to " << itr->getIp() << ":"
                << itr->getPort() << ", session = " << sessionId
                << std::endl;
      s->connect(itr->getIp(), itr->getPort(), sessionId, this);
    }
  }

  /// @overload Initiator
  void block() throw ( ConfigError, RuntimeError )
  {

  }

  /// @overload Initiator
  void stop(double timeout)
  {
    VLOG(VLOG_LEVEL_IBAPI_SOCKET_INITIATOR)
        << "Stopping connector with timeout = "
        << timeout << std::endl;
  }

  /// @overload Initiator
  void stop(bool force) {
    VLOG(VLOG_LEVEL_IBAPI_SOCKET_INITIATOR)
        << "Stopping connector with force = "
        << force << std::endl;
  }

  /// @overload Initiator
  bool isLoggedOn() {
    return false;
  }

  /// @implement SocketConnector::Strategy
  void onConnect(SocketConnector& connector, int clientId)
  {
    VLOG(VLOG_LEVEL_IBAPI_SOCKET_INITIATOR)
        << "Connection (" << clientId << ") established." << std::endl;
  }

  /// @implement SocketConnector::Strategy
  void onData(SocketConnector& connector, int clientId)
  {

  }

  /// @implement SocketConnector::Strategy
  void onError(SocketConnector& connector)
  {

  }

  /// @implement SocketConnector::Strategy
  void onDisconnect(SocketConnector& connector, int clientId)
  {
    VLOG(VLOG_LEVEL_IBAPI_SOCKET_INITIATOR)
        << "Connection (" << clientId << ") disconnected." << std::endl;
  }

  /// @implement SocketConnector::Strategy
  void onTimeout(SocketConnector& connector)
  {

  }


 private:
  Application& application_;
  std::list<SessionSetting>& sessionSettings_;
  std::map< SessionID, boost::shared_ptr<SocketConnector> > socketConnectors_;
};


SocketInitiator::SocketInitiator(Application& app,
                                 std::list<SessionSetting>& settings)
    : impl_(new SocketInitiatorImpl(app, settings))
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
  impl_->onConnect(connector, clientId);
}

/// @implement SocketConnector::Strategy
void SocketInitiator::onData(SocketConnector& connector, int clientId)
{
  impl_->onData(connector, clientId);
}

/// @implement SocketConnector::Strategy
void SocketInitiator::onError(SocketConnector& connector)
{
  impl_->onError(connector);
}

/// @implement SocketConnector::Strategy
void SocketInitiator::onTimeout(SocketConnector& connector)
{
  impl_->onTimeout(connector);
}

} // namespace ib

