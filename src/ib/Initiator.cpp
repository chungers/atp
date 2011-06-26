
#include <glog/logging.h>

#include "ib/Initiator.hpp"

namespace ib {

Initiator::Initiator(const string& host, unsigned int port, unsigned int id) {
}

void Initiator::onConnect() {
    boost::unique_lock<boost::mutex> lock(connected_mutex_);
    connected_ = true;
    connected_control_.notify_all();


}

void Initiator::onError(const int errorCode) {
  switch (errorCode) {
    case 326:
      LOG(WARNING) << "Conflicting connection id. Disconnecting.";
      select_client_->disconnect();
      break;
    case 509:
      LOG(WARNING) << "Connection reset. Disconnecting.";
      select_client_->disconnect();
      break;
    case 1100:
      LOG(WARNING) << "Error code = " << errorCode << " disconnecting.";
      select_client_->disconnect();
      break;
    case 502:
    default:
      LOG(WARNING) << "Unhandled Error = " << errorCode << ", do nothing.";
      break;
  }
}

 void Initiator::onHeartBeat(long time) {
   select_client_->received_heartbeat(time);
 }

}

