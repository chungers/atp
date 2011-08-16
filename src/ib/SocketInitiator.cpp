
#include <set>
#include <glog/logging.h>

#include <Shared/EWrapper.h>

#include "common.hpp"
#include "ib/SocketInitiator.hpp"
#include "ib/EWrapperFactory.hpp"


using ib::internal::EWrapperFactory;

namespace IBAPI {


class SocketInitiatorImpl : public SocketInitiator {

 public :
  SocketInitiatorImpl(Application& app, std::set<SessionSetting>& settings) :
      application_(app),
      sessionSettings_(settings),
      ewrapperFactoryPtr_(EWrapperFactory::getInstance()) {

  }

  ~SocketInitiatorImpl() {}

  /// @overload Initiator
  void start() throw ( ConfigError, RuntimeError )
  {
    
  }

  /// @overload Initiator
  void block() throw ( ConfigError, RuntimeError )
  {

  }

  /// @overload Initiator
  void stop(double timeout)
  {
    VLOG(VLOG_LEVEL_IBAPI_SOCKET_INITIATOR) << "Stopping connector with timeout = "
                                            << timeout << std::endl;
  }

  /// @overload Initiator
  void stop(bool force) {
    VLOG(VLOG_LEVEL_IBAPI_SOCKET_INITIATOR) << "Stopping connector with force = "
                                            << force << std::endl;
  }

  /// @overload Initiator
  bool isLoggedOn() {
    return false;
  }

  /// @implement SocketConnector::Strategy
  void onConnect(SocketConnector& connector, int clientId)
  {
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
  void onTimeout(SocketConnector& connector)
  {

  }

 private:
  Application& application_;
  std::set<SessionSetting>& sessionSettings_;
  boost::shared_ptr<EWrapperFactory> ewrapperFactoryPtr_;
};


SocketInitiator::SocketInitiator(Application& app,
                                 std::set<SessionSetting>& settings)
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

