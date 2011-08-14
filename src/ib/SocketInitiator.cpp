
#include <glog/logging.h>

#include <Shared/EWrapper.h>

#include "common.hpp"
#include "ib/SocketInitiator.hpp"
#include "ib/EWrapperFactory.hpp"



namespace IBAPI {


SocketInitiator::SocketInitiator(Application& app,
                                 SessionSetting& setting,
                                 EWrapperFactory& ewrapperFactory)
    : Initiator(app, setting), ewrapperFactory_(ewrapperFactory) {

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

void SocketInitiator::onConnect(SocketConnector& connector, int clientId) {
}

EWrapper* SocketInitiator::getEWrapperImpl() {
  return ewrapperFactory_.getImpl();
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

