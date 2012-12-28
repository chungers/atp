
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <zmq.hpp>
#include <stdlib.h>

#include "utils.hpp"
#include "common.hpp"
#include "varz/varz.hpp"
#include "zmq/Publisher.hpp"


DEFINE_bool(publisherIgnoreSignalInterrupt, true,
            "Ignores interrupt signal (zmq 2.0 default behavior).");

DEFINE_VARZ_bool(publisher_event_loop_stopped, false, "");
DEFINE_VARZ_bool(publisher_ignores_sig_interrupt, true, "");
DEFINE_VARZ_int32(publisher_sig_interrupts, 0, "");
DEFINE_VARZ_int64(publisher_bytes_sent, 0, "");
DEFINE_VARZ_int64(publisher_messages_sent, 0, "");


namespace atp {
namespace zmq {

Publisher::Publisher(const string& addr,
                     const string& publishAddr,
                     ::zmq::context_t* context) :
    addr_(addr),
    publishAddr_(publishAddr),
    context_(context),
    localContext_(false),
    ready_(false)
{
  VARZ_publisher_ignores_sig_interrupt = FLAGS_publisherIgnoreSignalInterrupt;

  // start thread
   thread_ = boost::shared_ptr<boost::thread>(new boost::thread(
       boost::bind(&Publisher::process, this)));

  // Wait here for the connection to be ready.
  boost::unique_lock<boost::mutex> lock(mutex_);
  while (!ready_) {
    isReady_.wait(lock);
  }

  LOG(INFO) << "Publisher is ready: " << publishAddr_;
}

Publisher::~Publisher()
{
  if (localContext_) {
    ZMQ_PUBLISHER_LOGGER << "Deleting local context " << context_;
    delete context_;
  }
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
  if (context_ == NULL) {
    // Create own context
    context_ = new ::zmq::context_t(1);
    localContext_ = true;
    ZMQ_PUBLISHER_LOGGER << "Created local context.";
  }

  ::zmq::socket_t inbound(*context_, ZMQ_PULL);

  try {
    inbound.bind(addr_.c_str());
  } catch (::zmq::error_t e) {
    LOG(FATAL) << "Cannot bind inbound at " << addr_ << ":"
               << e.what();
  }
  // start publish socket
  ::zmq::socket_t publish(*context_, ZMQ_PUB);

  try {
    publish.bind(publishAddr_.c_str());
  } catch (::zmq::error_t e) {
    LOG(FATAL) << "Cannot bind publish at " << publishAddr_ << ":"
               << e.what();
  }

  ZMQ_PUBLISHER_LOGGER
      << "Inbound @ " << addr_
      << ", Publish @ " << publishAddr_;

  {
    boost::lock_guard<boost::mutex> lock(mutex_);
    ready_ = true;
  }
  isReady_.notify_all();

  bool stop = false;
  while (!stop) {
    while (true) {
      ::zmq::message_t message;
#ifdef ZMQ_3X
      int more(0);
#else
      int64_t more(0);
#endif
      size_t more_size = sizeof(more);
      //  Process all parts of the message
      try {
        inbound.recv(&message);
        inbound.getsockopt( ZMQ_RCVMORE, &more, &more_size);

        VARZ_publisher_bytes_sent += publish.send(message,
                                                  more? ZMQ_SNDMORE: 0);
        VARZ_publisher_messages_sent++;
      } catch (::zmq::error_t e) {
        // Ignore signal 4 on linux which causes
        // the publisher/ connector to hang.  Ignoring the interrupts is
        // ZMQ 2.0 behavior which changed in 2.1.
        if (e.num() == 4 && FLAGS_publisherIgnoreSignalInterrupt) {
          VARZ_publisher_sig_interrupts++;
          LOG(ERROR) << "Ignoring error "
                     << e.num() << ", exception: " << e.what();
          stop = false;
        } else {
          LOG(ERROR) << "Stopping on error "
                     << e.num() << ", exception: " << e.what();
          stop = true;
          break;
        }
      }

      if (!more) {
        break;      //  Last message part
      }
    }
  }

  VARZ_publisher_event_loop_stopped = true;

  LOG(ERROR) << "Publisher thread stopped.";
}


} // namespace zmq
} // namespace atp
