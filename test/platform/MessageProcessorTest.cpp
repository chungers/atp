

#include <string>

#include <boost/bind.hpp>

#include <zmq.hpp>

#include <gtest/gtest.h>
#include <glog/logging.h>

#include "utils.hpp"
#include "zmq/ZmqUtils.hpp"

#include "platform/message_processor.hpp"

using std::string;
using atp::platform::message_processor;


typedef boost::uint64_t timer;


const string PUB_ENDPOINT = "tcp://127.0.0.1:4444";

/// Simple handler
struct work_functor
{
  bool operator()(const string& topic, const string& message)
  {
    timer now = now_micros();
    timer sent = boost::lexical_cast<timer>(message);

    count++;
    LOG(INFO) << "functor=" << id
              << ", count="<< count
              << ", topic=" << topic
              << ", sent=" << sent
              << ", now=" << now
              << ", dt=" << (now - sent);
    return true;
  }
  string id;
  size_t count;
};

bool nflx(const string& topic, const string& message, size_t* count)
{
  (*count)++;
  LOG(INFO) << "nflx: " << topic << "," << message << ", count = " << *count;
  return true;
}

struct stop_functor
{
  bool operator()(const string& topic, const string& message)
  {
    LOG(INFO) << "STOP";
    return false;
  }
};

bool stop_function(const string& topic, const string& message,
                   const string& label)
{
  LOG(INFO) << "Stopping with " << label;
  return false;
}

TEST(MessageProcessorTest, UsageSyntax)
{
  // create and register handlers

  work_functor w1, w2;
  stop_functor s;

  w1.id = "AAPL.STK", w1.count = 0;
  w2.id = "GOOG.STK", w2.count = 0;

  message_processor::protobuf_handlers_map handlers;
  handlers.register_handler("AAPL.STK", w1);
  handlers.register_handler("GOOG.STK", w2);

  handlers.register_handler("STOP",
                            boost::bind(&stop_function, _1, _2, "hello"));

  // trivial state to be bound into the handler
  size_t nflx_count = 0;
  handlers.register_handler("NFLX.STK",
                            boost::bind(&nflx, _1, _2, &nflx_count));

  // create the message processor -- runs a new thread
  message_processor subscriber(PUB_ENDPOINT, handlers);


  // create the message publisher
  ::zmq::context_t ctx(1);
  ::zmq::socket_t socket(ctx, ZMQ_PUB);
  try {

    socket.bind(PUB_ENDPOINT.c_str());
    LOG(INFO) << "Bound to " << PUB_ENDPOINT;

  } catch (::zmq::error_t e) {
    LOG(ERROR) << "Cannot bind @ " << PUB_ENDPOINT;
    FAIL();
  }

  size_t count = 250;
  int messages = 0;

  timer start = now_micros();


  while (count--) {
    string topic(count % 2 ? "AAPL.STK" : "NFLX.STK");
    string message = boost::lexical_cast<string>(now_micros());

    LOG(INFO) << count << " sending " << topic << "@" << message;

    atp::zmq::send_copy(socket, topic, true);
    atp::zmq::send_copy(socket, message, false);

    usleep(20000);
    messages++;
  }

  // Stop
  atp::zmq::send_copy(socket, "STOP", true);
  atp::zmq::send_copy(socket, "STOP", false);
  LOG(INFO) << "Sent stop";

  LOG(INFO) << "nflx " << nflx_count;
  EXPECT_GT(nflx_count, 0);

  subscriber.block();
}
