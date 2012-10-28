

#include <string>

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
  void operator()(const string& topic, const string& message)
  {
    LOG(INFO) << "received: " << topic << "@" << message;

    timer now = now_micros();
    timer sent = boost::lexical_cast<timer>(message);

    count++;
    LOG(INFO) << "functor=" << id
              << ", count="<< count
              << ", topic=" << topic
              << ", dt=" << (now - sent);
  }
  string id;
  size_t count;
};

TEST(MessageProcessorTest, UsageSyntax)
{
  // create and register handlers

  work_functor w1, w2;

  w1.id = "AAPL.STK", w1.count = 0;
  w2.id = "GOOG.STK", w2.count = 0;

  message_processor::protobuf_handlers_map handlers;
  handlers.register_handler("AAPL.STK", w1);
  handlers.register_handler("GOOG.STK", w2);

  // create the message processor -- runs a new thread
  message_processor subscriber(PUB_ENDPOINT, handlers, 5);


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

  size_t count = 5000;
  while (count--) {
    string topic(count % 2 ? "AAPL.STK" : "GOOG.STK");
    string message = boost::lexical_cast<string>(now_micros());

    atp::zmq::send_copy(socket, topic, true);
    atp::zmq::send_copy(socket, message, false);

    LOG(INFO) << count << " sent " << topic << "@" << message;
  }

  sleep(10); // need to let the executor run a bit so work actually gets done

  EXPECT_EQ(count/2, w1.count);
  EXPECT_EQ(count/2, w2.count);
}
