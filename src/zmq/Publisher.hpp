#ifndef ATP_ZMQ_PUBLISHER_H_
#define ATP_ZMQ_PUBLISHER_H_

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <zmq.hpp>

#include "common.hpp"

namespace atp {
namespace zmq {


/// Simple publisher that runs in its own thread while
/// exposing another socket that allows clients to push
/// messages for publish.
class Publisher
{
 public:

  // addr - endpoint to connect to send messages for publish
  // publishAddr - endpoint for subscribers.
  Publisher(const std::string& addr, const std::string& publishAddr,
            ::zmq::context_t* context = NULL);
  ~Publisher();

  const std::string& addr();
  const std::string& publishAddr();

  void block();

 protected:
  void process();

 private:
  const std::string& addr_;
  const std::string& publishAddr_;
  boost::shared_ptr<boost::thread> thread_;
  boost::shared_ptr< ::zmq::context_t > context_;
  bool ready_;
  boost::mutex mutex_;
  boost::condition_variable isReady_;
};


} // namespace zmq
} // namespace atp

#endif //ATP_ZMQ_PUBLISHER_H_
