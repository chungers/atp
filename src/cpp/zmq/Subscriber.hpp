#ifndef ATP_ZMQ_SUBSCRIBER_H_
#define ATP_ZMQ_SUBSCRIBER_H_

#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <zmq.hpp>

#include "common.hpp"

using std::string;
using std::vector;
using ::zmq::context_t;
using ::zmq::error_t;
using ::zmq::socket_t;

namespace atp {
namespace zmq {


class Subscriber : NoCopyAndAssign
{
 public:

  struct Strategy : NoCopyAndAssign {

    virtual bool check_message(::zmq::socket_t& socket) = 0;

  };


  Subscriber(const string& publisher_endpoint,
             const vector<string>& subscriptions,
             Strategy& strategy,
             context_t* context = NULL);

  ~Subscriber();

  const string& publisher_addr() const;

  void block();

 protected:
  void process();

 private:
  const string publisher_endpoint_;
  const vector<string> subscriptions_;
  Strategy& strategy_;
  context_t* context_;
  boost::shared_ptr<boost::thread> thread_;
  boost::mutex mutex_;
  bool ready_;
  boost::condition_variable isReady_;

};

} // zmq
} // atp

#endif //ATP_ZMQ_SUBSCRIBER_H_
