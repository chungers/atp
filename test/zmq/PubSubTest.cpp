
#include <string>
#include <iostream>
#include <vector>

#include <gtest/gtest.h>

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include "common.hpp"
#include "zmq/Publisher.hpp"
#include "zmq/Subscriber.hpp"
#include "zmq/ZmqUtils.hpp"


using namespace atp::zmq;


#define LOGGER LOG(INFO)


struct TestStrategy : public Subscriber::Strategy
{
  TestStrategy(size_t howMany = 1) : howMany(howMany) {}


  virtual bool check_message(::zmq::socket_t& socket)
  {
    std::string buff;
    bool status = true;

    LOG(INFO) << "About to receive @ socket " << &socket;
    bool ok = atp::zmq::receive(socket, &buff);

    LOG(INFO) << "status = " << ok << ", received: " << buff;

    if (buff == "stop") {
      LOG(INFO) << "stopping";
      status = false;
    } else {
      messages.push_back(buff);
    }
    return status;
  }

  size_t howMany;
  std::vector<string> messages;
};

using namespace std;

TEST(PubSubTest, PubSubSingleFrameTest)
{
  LOGGER << "Current TimeMicros = " << now_micros() << std::endl;

  atp::zmq::Version version;
  LOG(INFO) << "ZMQ " << version;


  int messages = 50;

  int port = 7771;
  const std::string& addr = atp::zmq::EndPoint::tcp(port);

  // Now start publisher - proxy
  int push_port = 7772;
  const std::string& push_addr = atp::zmq::EndPoint::tcp(push_port);

  Publisher pub(push_addr, addr);
  LOG(INFO) << "Started publisher / proxy";

  // first start subscriber
  vector<string> subscriptions;
  subscriptions.push_back("event");
  subscriptions.push_back("stop");
  TestStrategy testStrategy(messages);
  Subscriber sub(addr, subscriptions, testStrategy);
  LOG(INFO) << "Started subscriber";

  LOG(INFO) << "Starting push client to push events.";
  ::zmq::context_t context(1);
  ::zmq::socket_t push(context, ZMQ_PUSH);
  push.connect(push_addr.c_str());
  LOG(INFO) << "push client connected to " << push_addr
            << " for publishing on " << addr;

  sleep(2);

  vector<string> sent;
  for (int i = 0; i < messages; ++i) {
    string msg("event-" + boost::lexical_cast<string>(i));
    size_t out = atp::zmq::send_copy(push, msg, false);
    LOG(INFO) << "Sent message: " << i << ',' << msg << ',' << out;
    sent.push_back(msg);
  }

  sleep(1);

  LOG(INFO) << "Sending stop";
  atp::zmq::send_copy(push, "stop", false);

  sleep(1);

  LOG(INFO) << "Blocking for subscriber's completion.";
  sub.block();
}

