
#include <set>

#include <glog/logging.h>

#include <Shared/EWrapper.h>

#include "common.hpp"
#include "ib/SocketInitiator.hpp"
#include "ib/EWrapperFactory.hpp"



namespace IBAPI {


SocketInitiator::SocketInitiator(Application& app,
                                 std::set<SessionSetting>& settings,
                                 EWrapperFactory& ewrapperFactory)
    : Initiator(app, settings), ewrapperFactory_(ewrapperFactory) {

}

void SocketInitiator::start() throw ( ConfigError, RuntimeError ) {
  VLOG(VLOG_LEVEL_IBAPI_SOCKET_INITIATOR) << "Starting connector. " << std::endl;
}

void SocketInitiator::block() throw ( ConfigError, RuntimeError ) {
}

void SocketInitiator::stop(double timeout) {
  VLOG(VLOG_LEVEL_IBAPI_SOCKET_INITIATOR) << "Stopping connector with timeout = "
                   << timeout << std::endl;
}

void SocketInitiator::stop(bool force) {
  VLOG(VLOG_LEVEL_IBAPI_SOCKET_INITIATOR) << "Stopping connector with force = "
                   << force << std::endl;
}

bool SocketInitiator::isLoggedOn() {
  return false;
}

/// @implement SocketConnector::Strategy
void SocketInitiator::onConnect(SocketConnector& connector, int clientId) {
}

/// @implement SocketConnector::Strategy
void SocketInitiator::onData(SocketConnector& connector, int clientId) {
}

/// @implement SocketConnector::Strategy
void SocketInitiator::onError(SocketConnector& connector) {
}

/// @implement SocketConnector::Strategy
void SocketInitiator::onTimeout(SocketConnector& connector) {

}

} // namespace ib

