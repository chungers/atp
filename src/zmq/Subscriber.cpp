
#include <boost/bind.hpp>

#include "log_levels.h"

#include "zmq/Subscriber.hpp"


using std::string;
using std::vector;
using ::zmq::context_t;
using ::zmq::error_t;
using ::zmq::socket_t;

namespace atp {
namespace zmq {



Subscriber::Subscriber(const string& publisher_endpoint,
                       const vector<string>& subscriptions,
                       Strategy& strategy,
                       context_t* context) :
    publisher_endpoint_(publisher_endpoint),
    subscriptions_(subscriptions),
    strategy_(strategy),
    context_(context),
    ready_(false)
{
  // start thread
  thread_ = boost::shared_ptr<boost::thread>(new boost::thread(
      boost::bind(&Subscriber::process, this)));

  // Wait here for the connection to be ready.
  boost::unique_lock<boost::mutex> lock(mutex_);
  while (!ready_) {
    isReady_.wait(lock);
  }

  ZMQ_SUBSCRIBER_LOGGER << "Subscriber is ready.";
}

Subscriber::~Subscriber()
{
}

const string& Subscriber::publisher_addr() const
{
  return publisher_endpoint_;
}

void Subscriber::process()
{
  boost::lock_guard<boost::mutex> lock(mutex_);

  bool localContext = false;
  if (context_ == NULL) {
    // Create own context
    context_ = new context_t(1);
    localContext = true;
    ZMQ_SUBSCRIBER_LOGGER << "Created local context.";
  } else {
    ZMQ_SUBSCRIBER_LOGGER << "Using shared context: " << context_;
  }

  socket_t socket(*context_, ZMQ_SUB);

  bool connected = false;
  try {

    socket.connect(publisher_endpoint_.c_str());
    connected = true;
    ZMQ_SUBSCRIBER_LOGGER << "Connected to publisher " << publisher_endpoint_;

  } catch (error_t e) {
    LOG(FATAL) << "Cannot connect " << publisher_endpoint_ << ":" << e.what();
  }

  // now add subscriptions
  try {

    for (vector<string>::const_iterator topic = subscriptions_.begin();
         topic != subscriptions_.end();
         ++topic) {
      socket.setsockopt(ZMQ_SUBSCRIBE,
                        topic->c_str(),
                        topic->length());
      ZMQ_SUBSCRIBER_LOGGER << "subscribed to topic = " << *topic;
    }

  } catch (error_t e) {
    LOG(FATAL) << "Cannot add subscriptions: " << e.what();
  }

  ready_ = connected;
  isReady_.notify_all();

  if (connected) {
    try {
      while (strategy_.check_message(socket)) {
        // just loop
      }

      ZMQ_SUBSCRIBER_LOGGER << "Stopping after strategy returns false.";

    } catch (::zmq::error_t e) {
      LOG(ERROR) << "Exception while processing messages: " << e.what()
                 << ", stopping.";
    }
  }

  if (localContext && context_ != NULL) delete context_;
  LOG(ERROR) << "Subscriber thread stopped.";
}

} // zmq
} // atp
