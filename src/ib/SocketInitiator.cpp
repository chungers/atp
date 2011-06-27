
#include <glog/logging.h>

#include <Shared/EWrapper.h>
#include "ib/SocketInitiator.hpp"
#include "ib/EventDispatcher.hpp"


namespace ib {

SocketInitiator::SocketInitiator(ib::Application& app,
                                 ib::SessionSetting& setting)
    : application_(app), setting_(setting) {

}

void SocketInitiator::start() {
  // Create the socket connector
  // Create callbacks
}

void SocketInitiator::onConnect() {
    boost::unique_lock<boost::mutex> lock(connected_mutex_);
    connected_ = true;
    connected_control_.notify_all();
}

void SocketInitiator::onError(const unsigned int errorCode) {
  switch (errorCode) {
    case 326:
      LOG(WARNING) << "Conflicting connection id. Disconnecting.";
      socket_connector_->disconnect();
      break;
    case 509:
      LOG(WARNING) << "Connection reset. Disconnecting.";
      socket_connector_->disconnect();
      break;
    case 1100:
      LOG(WARNING) << "Error code = " << errorCode << " disconnecting.";
      socket_connector_->disconnect();
      break;
    case 502:
    default:
      LOG(WARNING) << "Unhandled Error = " << errorCode << ", do nothing.";
      break;
  }
}

void SocketInitiator::onHeartBeat(long time) {
  socket_connector_->updateHeartbeat(time);
}

EWrapper* SocketInitiator::getEWrapperImpl() {
  return NULL;
}

} // namespace ib

