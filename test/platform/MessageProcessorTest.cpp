

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


struct goog
{
  goog() : count(0) {}

  bool process(const string& topic, const string& message)
  {
    count++;
    LOG(INFO) << "goog:" << topic << "," << message << "," << count;
    return true;
  }

  size_t count;
};

struct aapl_mp
{
  aapl_mp(const string& ep) : count(0)
  {
    handlers.register_handler("AAPL.STK",
                              boost::bind(&aapl_mp::process, this, _1, _2));
    handlers.register_handler("STOP",
                              boost::bind(&aapl_mp::stop, this, _1, _2));
    mp = new message_processor(ep, handlers);
  }

  ~aapl_mp()
  {
    delete mp;
  }

  bool process(const string& topic, const string& message)
  {
    count++;
    LOG(INFO) << "appl_mp:" << topic << "," << message << "," << count;
    return true;
  }

  bool stop(const string& topic, const string& message)
  {
    LOG(INFO) << "aapl_mp stopping.";
    return false;
  }

  size_t count;
  message_processor::protobuf_handlers_map handlers;
  message_processor* mp;
};

TEST(MessageProcessorTest, UsageSyntax)
{
  // create a special message processor
  aapl_mp aapl_watcher(PUB_ENDPOINT);

  // create and register handlers
  message_processor::protobuf_handlers_map handlers;
  goog g;
  handlers.register_handler("GOOG.STK",
                            boost::bind(&goog::process, &g, _1, _2));
  handlers.register_handler("STOP",
                            boost::bind(&stop_function, _1, _2, "hello"));

  // trivial state to be bound into the handler
  size_t nflx_count = 0;
  handlers.register_handler("NFLX.STK",
                            boost::bind(&nflx, _1, _2, &nflx_count));

  // create the message processor -- runs a new thread
  message_processor subscriber(PUB_ENDPOINT, handlers);

  LOG(INFO) <<  "Started watchers.  Now creating the publisher.";

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

  size_t nflx_sent = 0;
  size_t goog_sent = 0;
  size_t aapl_sent = 0;
  while (count--) {

    string topic;
    switch (count % 3) {
      case 0 : topic = "AAPL.STK"; aapl_sent++; break;
      case 1 : topic = "NFLX.STK"; nflx_sent++; break;
      case 2 : topic = "GOOG.STK"; goog_sent++; break;
    }
    string message = boost::lexical_cast<string>(now_micros());

    LOG(INFO) << count << " sending " << topic << "@" << message;

    atp::zmq::send_copy(socket, topic, true);
    atp::zmq::send_copy(socket, message, false);

    usleep(10000);
    messages++;
  }

  // Stop
  atp::zmq::send_copy(socket, "STOP", true);
  atp::zmq::send_copy(socket, "STOP", false);
  LOG(INFO) << "Sent stop";

  LOG(INFO) << "aapl got " << aapl_watcher.count << ", aapl sent " << aapl_sent;
  LOG(INFO) << "goog got " << g.count << ", goog sent " << goog_sent;
  LOG(INFO) << "nflx got " << nflx_count << ", nflx sent " << nflx_sent;
  EXPECT_GE(aapl_sent, aapl_watcher.count);
  EXPECT_GE(nflx_sent, nflx_count);
  EXPECT_GE(goog_sent, g.count);

  subscriber.block();
}
