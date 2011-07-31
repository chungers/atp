
#include <glog/logging.h>

#include <Shared/EWrapper.h>
#include "ib/SocketInitiator.hpp"
#include "ib/EventDispatcher.hpp"



namespace IBAPI {


const int VLOG_LEVEL = 2;


SocketInitiator::SocketInitiator(Application& app,
                                 SessionSetting& setting)
    : Initiator(app, setting) {

}

void SocketInitiator::start() throw ( ConfigError, RuntimeError ) {
  VLOG(VLOG_LEVEL) << "Starting connector. " << std::endl;
}

void SocketInitiator::block() throw ( ConfigError, RuntimeError ) {
}

void SocketInitiator::stop(double timeout) {
  VLOG(VLOG_LEVEL) << "Stopping connector with timeout = "
                   << timeout << std::endl;
}

void SocketInitiator::stop(bool force) {
  VLOG(VLOG_LEVEL) << "Stopping connector with force = "
                   << force << std::endl;
}

bool SocketInitiator::isLoggedOn() {
  return false;
}

void SocketInitiator::onConnect(SocketConnector& connector, int clientId) {
}

EWrapper* SocketInitiator::getEWrapperImpl() {
  return new ib::internal::EventDispatcher(application_, *this);
}

void SocketInitiator::onError(SocketConnector& connector,
                              const int clientId,
                              const unsigned int errorCode) {
  switch (errorCode) {
    case 326:
      LOG(WARNING) << "Conflicting connection id. Disconnecting.";
      //socket_connector_->disconnect();
      break;
    case 509:
      LOG(WARNING) << "Connection reset. Disconnecting.";
      //socket_connector_->disconnect();
      break;
    case 1100:
      LOG(WARNING) << "Error code = " << errorCode << " disconnecting.";
      //socket_connector_->disconnect();
      break;
    case 502:
    default:
      LOG(WARNING) << "Unhandled Error = " << errorCode << ", do nothing.";
      break;
  }
}

void SocketInitiator::onTimeout(const long time) {

}

} // namespace ib

