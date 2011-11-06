
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <glog/logging.h>
#include <zmq.hpp>
#include <stdlib.h>

#include "utils.hpp"
#include "common.hpp"
#include "zmq/Publisher.hpp"


namespace atp {
namespace zmq {

Publisher::Publisher(const string& addr,
                     const string& publishAddr,
                     ::zmq::context_t* context) :
    addr_(addr),
    publishAddr_(publishAddr),
    context_(context),
    ready_(false)
{
  // start thread
  thread_ = boost::shared_ptr<boost::thread>(new boost::thread(
      boost::bind(&Publisher::process, this)));

  // Wait here for the connection to be ready.
  boost::unique_lock<boost::mutex> lock(mutex_);
  while (!ready_) {
    isReady_.wait(lock);
  }

  LOGGER_ZMQ_PUBLISHER << "Publisher is ready.";
}

Publisher::~Publisher()
{
}

const std::string& Publisher::addr()
{
  return addr_;
}

const std::string& Publisher::publishAddr()
{
  return publishAddr_;
}

void Publisher::block()
{
  thread_->join();
}

void Publisher::process()
{
  if (context_.get() == NULL) {
    // Create own context
    ::zmq::context_t context(1);
    context_.reset(&context);
  }

  // start message receiver socket
  ::zmq::socket_t inbound(*context_, ZMQ_PULL);
  inbound.bind(addr_.c_str());

  // start publish socket
  ::zmq::socket_t publish(*context_, ZMQ_PUB);
  publish.bind(publishAddr_.c_str());

  LOGGER_ZMQ_PUBLISHER
      << "Inbound @ " << addr_
      << ", Publish @ " << publishAddr_;

  {
    boost::lock_guard<boost::mutex> lock(mutex_);
    ready_ = true;
  }
  isReady_.notify_all();

  while (1) {
    while (1) {
      ::zmq::message_t message;
      int64_t more;
      size_t more_size = sizeof(more);
      //  Process all parts of the message
      inbound.recv(&message);
      inbound.getsockopt( ZMQ_RCVMORE, &more, &more_size);

      LOGGER_ZMQ_PUBLISHER << "Published " << message.size() << " bytes: "
                           << static_cast<char*>(message.data());

      publish.send(message, more? ZMQ_SNDMORE: 0);

      if (!more)
        break;      //  Last message part
    }
  }
  LOG(ERROR) << "Publisher thread stopped." << std::endl;
}


} // namespace zmq
} // namespace atp
