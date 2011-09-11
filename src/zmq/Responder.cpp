
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <glog/logging.h>
#include <zmq.hpp>
#include <stdlib.h>

#include "utils.hpp"
#include "common.hpp"

#include "zmq/Responder.hpp"

namespace atp {
namespace zmq {

void free_func(void* mem, void* mem2)
{
  VLOG(VLOG_LEVEL_ZMQ_RESPONDER) << "Freeing memory: " << mem
                                 << ", " << mem2 << std::endl;
}

#define LOGGER VLOG(VLOG_LEVEL_ZMQ_RESPONDER)

Responder::Responder(const string& addr,
                     SocketReader& reader,
                     SocketWriter& writer) :
    context_(1),
    socket_(context_, ZMQ_REP),
    running_(false),
    reader_(reader),
    writer_(writer) {

  socket_.bind(addr.c_str());
  // start thread
  thread_ = boost::shared_ptr<boost::thread>(new boost::thread(
      boost::bind(&Responder::process, this)));
  running_ = true;

  LOG(INFO) << "Started responder. " << std::endl;
}

Responder::~Responder()
{
}


::zmq::context_t& Responder::context()
{
  return context_;
}

void Responder::stop()
{
  boost::unique_lock<boost::mutex> lock(mutex_);
  running_ = false;
  LOG(INFO) << "Closing socket" << std::endl;
  socket_.close();
  LOG(INFO) << "Joining thread" << std::endl;
  thread_->join();
}

void Responder::process()
{
  bool readOk = true;
  bool writeOk = true;
  while (running_) {
    readOk = reader_(socket_);
    LOG(INFO) << "Read was " << readOk << std::endl;
    if (readOk) {
      writeOk = writer_(socket_);
      LOG(INFO) << "Write was " << writeOk << std::endl;
      if (!writeOk) {
        break;
      }
    } else {
      break;
    }
    LOG(INFO) << "running=" << running_ << std::endl;
  }
  LOG(ERROR) << "Responder listening thread stopped." << std::endl;
}


} // namespace zmq
} // namespace atp
