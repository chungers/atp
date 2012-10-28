

#include <boost/bind.hpp>
#include <boost/thread.hpp>

#include <glog/logging.h>


#include "common.hpp"
#include "common/executor.hpp"
#include "zmq/ZmqUtils.hpp"

#include "platform/message_processor.hpp"

using std::string;
using boost::condition_variable;
using boost::mutex;
using boost::scoped_ptr;
using boost::thread;

using atp::common::executor;

namespace atp {
namespace platform {

using atp::platform::message_processor;


class message_processor::implementation : NoCopyAndAssign
{
 public:

  typedef boost::uint64_t timestamp_t;

  implementation(const string& endpoint,
                 const message_processor::protobuf_handlers_map& handlers,
                 ::zmq::context_t* context,
                 size_t threads) :
    endpoint_(endpoint),
    handlers_(handlers),
    context_(context == NULL ? new ::zmq::context_t(1) : context),
    executor_(threads),
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

  struct work
  {
    work(const message_processor::protobuf_handlers_map& handlers) :
        handlers(handlers),
        submit(0), process_start(0), process_complete(0)
    {
    }

    void operator()()
    {
      process_start = now_micros();
      handlers.process_raw_message(topic, message);
      process_complete = now_micros();
      timestamp_t schedule_delay = process_start - submit;
      LOG(INFO) << "..... " << schedule_delay;
    }

    const message_processor::protobuf_handlers_map& handlers;
    string topic, message;
    timestamp_t submit;
    timestamp_t process_start;
    timestamp_t process_complete;
  };


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

    while (true) {

      work work(handlers_);

      if (atp::zmq::receive(socket, &(work.topic)) &&
          !atp::zmq::receive(socket, &(work.message))) {

        if (handlers_.is_supported(work.topic)) {
          work.submit = now_micros();
          executor_.Submit(work);
        }
      } else {
        LOG(INFO) << "Got instead " << work.topic << "@" << work.message;
      }
    }
  }

  string endpoint_;
  const message_processor::protobuf_handlers_map& handlers_;
  ::zmq::context_t* context_;
  executor executor_;
  bool own_context_;
  bool ready_;
  scoped_ptr<thread> listener_thread_;
  mutex mutex_;
  condition_variable until_ready_;
};


message_processor::message_processor(const string& endpoint,
                                     const protobuf_handlers_map& handlers,
                                     size_t threads,
                                     ::zmq::context_t* context) :
    impl_(new implementation(endpoint, handlers, context, threads))
{
}

message_processor::~message_processor()
{
}

} // platform
} // atp
