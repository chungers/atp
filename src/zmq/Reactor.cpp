
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <glog/logging.h>
#include <zmq.hpp>
#include <stdlib.h>

#include "utils.hpp"
#include "common.hpp"
#include "zmq/Reactor.hpp"

namespace atp {
namespace zmq {

Reactor::Reactor(const string& addr,
                 Reactor::Strategy& strategy) :
    addr_(addr),
    strategy_(strategy),
    ready_(false)
{
  // start thread
  thread_ = boost::shared_ptr<boost::thread>(new boost::thread(
      boost::bind(&Reactor::process, this)));

  // Wait here for the connection to be ready.
  boost::unique_lock<boost::mutex> lock(mutex_);
  while (!ready_) {
    isReady_.wait(lock);
  }

  ZMQ_REACTOR_LOGGER << "Reactor is ready." << std::endl;
}

Reactor::~Reactor()
{
}

const std::string& Reactor::addr()
{
  return addr_;
}

void Reactor::block()
{
  thread_->join();
}

void Reactor::process()
{
  // Note that the context and socket are all local variables.
  // This is because this method is running in a separate thread
  // from the caller of the constructor, and we want to dedicate
  // one context and one socket per thread while making them
  // hidden from the caller.  In general, we don't want to create
  // context and socket in one thread and then have another thread
  // receiving or sending on them.
  ::zmq::context_t context(1);

  int socketType = strategy_.socketType();
  switch (socketType) {
    case ZMQ_PULL : ZMQ_REACTOR_LOGGER << "ZMQ_PULL/" << ZMQ_PULL;
      break;
    case ZMQ_REP : ZMQ_REACTOR_LOGGER << "ZMQ_REP/" << ZMQ_REP;
      break;
    default : LOG(FATAL) << "NOT SUPPORTED";
  }

  ::zmq::socket_t socket(context, socketType);

  try {
    socket.bind(addr_.c_str());
    ZMQ_REACTOR_LOGGER << "listening @ " << addr_;
  } catch (::zmq::error_t e) {
    LOG(FATAL) << "Cannot bind " << addr_ << ":" << e.what();
  }

  {
    boost::lock_guard<boost::mutex> lock(mutex_);
    ready_ = true;
  }
  isReady_.notify_all();

  while (strategy_.respond(socket)) {}
  LOG(ERROR) << "Reactor listening thread stopped." << std::endl;
}


} // namespace zmq
} // namespace atp
