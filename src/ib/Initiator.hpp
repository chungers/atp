#ifndef IB_INITIATOR_H_
#define IB_INITIATOR_H_

#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>

#include "ib/EventDispatcher.hpp"
#include "ib/SocketConnector.hpp"

using ib::internal::EventDispatcher;
using ib::internal::SocketConnector;

namespace ib {

class Initiator {

 public:


  Initiator(const string& host, unsigned int port, unsigned int id);
  ~Initiator();


 private:

  void onConnect();
  void onError(const int errorCode);
  void onHeartBeat(long time);

 private:
  boost::scoped_ptr<SocketConnector> select_client_;

  volatile bool connected_;
  boost::mutex connected_mutex_;
  boost::condition_variable connected_control_;

};

}

#endif // IB_INITIATOR_H_
