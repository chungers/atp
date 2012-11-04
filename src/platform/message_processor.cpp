

#include <boost/bind.hpp>
#include <boost/thread.hpp>

#include <glog/logging.h>


#include "common.hpp"
#include "zmq/ZmqUtils.hpp"

#include "platform/message_processor.hpp"

using std::string;
using boost::condition_variable;
using boost::mutex;
using boost::scoped_ptr;
using boost::thread;


namespace atp {
namespace platform {

using atp::platform::message_processor;


class message_processor::implementation : NoCopyAndAssign
{
 public:

  implementation(const string& endpoint,
                 const message_processor::protobuf_handlers_map& handlers,
                 ::zmq::context_t* context) :
    endpoint_(endpoint),
    handlers_(handlers),
    context_(context == NULL ? new ::zmq::context_t(1) : context),
    own_context_(context == NULL),
    ready_(false),
    listener_thread_(new thread(boost::bind(&implementation::run, this)))
  {
    boost::unique_lock<mutex> lock(mutex_);
    while (!ready_) until_ready_.wait(lock);
  }

  ~implementation()
  {
  }


  void block()
  {
    listener_thread_->join();
  }

 private:

  void run()
  {
    ::zmq::socket_t socket(*context_, ZMQ_SUB);

    try {

      socket.connect(endpoint_.c_str());
      LOG(INFO) << "Connected to " << endpoint_;

    } catch (::zmq::error_t e) {
      LOG(ERROR) << "Cannot connect to " << endpoint_;
      return;
    }

    // Add subscriptions by the handler key
    message_processor::protobuf_handlers_map::message_keys_itr itr;
    for (itr = handlers_.begin(); itr != handlers_.end(); ++itr) {
      string topic = itr->first;
      socket.setsockopt(ZMQ_SUBSCRIBE, topic.c_str(), topic.length());
      LOG(INFO) << "subscribed to " << topic;
    }

    {
      boost::lock_guard<mutex> lock(mutex_);
      ready_ = true;
    }
    until_ready_.notify_all();

    LOG(INFO) << "Ready to handle messages.";

    string topic, message;
    topic.reserve(20);
    message.reserve(200);

    bool run = true;
    while (run) {

      if (atp::zmq::receive(socket, &topic) &&
          !atp::zmq::receive(socket, &message)) {
        run = handlers_.process_raw_message(topic, message);
      } else {
        LOG(INFO) << "Got instead " << topic << "@" << message;
      }
    }

    LOG(INFO) << "Listening thread stopped.";
  }

  string endpoint_;
  const message_processor::protobuf_handlers_map& handlers_;
  ::zmq::context_t* context_;
  bool own_context_;
  bool ready_;
  scoped_ptr<thread> listener_thread_;
  mutex mutex_;
  condition_variable until_ready_;
};


message_processor::message_processor(const string& endpoint,
                                     const protobuf_handlers_map& handlers,
                                     ::zmq::context_t* context) :
    impl_(new implementation(endpoint, handlers, context))
{
}

message_processor::~message_processor()
{
}

void message_processor::block()
{
  impl_->block();
}

} // platform
} // atp
