
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <glog/logging.h>
#include <zmq.hpp>
#include <stdlib.h>

#include "utils.hpp"
#include "common.hpp"
#include "zmq/Responder.hpp"

#define LOGGER VLOG(VLOG_LEVEL_ZMQ_RESPONDER)

namespace atp {
namespace zmq {

Responder::Responder(const string& addr,
                     SocketReader& reader,
                     SocketWriter& writer) :
    addr_(addr),
    reader_(reader),
    writer_(writer),
    ready_(false)
{
  // start thread
  thread_ = boost::shared_ptr<boost::thread>(new boost::thread(
      boost::bind(&Responder::process, this)));

  // Wait here for the connection to be ready.
  boost::unique_lock<boost::mutex> lock(mutex_);
  while (!ready_) {
    isReady_.wait(lock);
  }

  LOG(INFO) << "Responder is ready." << std::endl;
}

Responder::~Responder()
{
}

const std::string& Responder::addr()
{
  return addr_;
}


void Responder::process()
{
  // Note that the context and socket are all local variables.
  // This is because this method is running in a separate thread
  // from the caller of the constructor, and we want to dedicate
  // one context and one socket per thread while making them
  // hidden from the caller.  In general, we don't want to create
  // context and socket in one thread and then have another thread
  // receiving or sending on them.
  ::zmq::context_t context(1);
  ::zmq::socket_t socket(context, ZMQ_REP);
  socket.bind(addr_.c_str());
  LOG(INFO) << "ZMQ_REP listening @ " << addr_ << std::endl;

  {
    boost::lock_guard<boost::mutex> lock(mutex_);
    ready_ = true;
  }
  isReady_.notify_all();

  while (reader_.receive(socket) && writer_.send(socket)) {}
  LOG(ERROR) << "Responder listening thread stopped." << std::endl;
}


} // namespace zmq
} // namespace atp
