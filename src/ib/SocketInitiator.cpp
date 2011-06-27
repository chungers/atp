
#include <glog/logging.h>

#include <Shared/EWrapper.h>
#include "ib/SocketInitiator.hpp"
#include "ib/EventDispatcher.hpp"


namespace ib {


const int VLOG_LEVEL = 2;



SocketInitiator::SocketInitiator(ib::Application& app,
                                 ib::SessionSetting& setting)
    : application_(app), setting_(setting) {

}

void SocketInitiator::start() {
  VLOG(VLOG_LEVEL) << "Starting connector. " << std::endl;

  // Create the socket connector
  socket_connector_.reset(new SocketConnector(setting_.getConnectionId()));

  // Start the connector.  This starts a new thread
  socket_connector_->connect(setting_.getHost(), setting_.getPort(), this);
}

void SocketInitiator::stop(double timeout) {
  VLOG(VLOG_LEVEL) << "Stopping connector with timeout = "
                   << timeout << std::endl;
  socket_connector_->disconnect();
  socket_connector_->stop();
}

void SocketInitiator::stop(bool force) {
  VLOG(VLOG_LEVEL) << "Stopping connector with force = "
                   << force << std::endl;
  socket_connector_->disconnect();
  socket_connector_->stop();
}

void SocketInitiator::block() {
  // Wait for the thread to join.
  socket_connector_->join();
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
  return new ib::internal::EventDispatcher(application_, *this);
}

} // namespace ib

