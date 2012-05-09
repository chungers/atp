
#include <algorithm>
#include <iostream>
#include <map>
#include <list>
#include <vector>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <gtest/gtest.h>

#include "ib/Application.hpp"
#include "ib/EWrapperFactory.hpp"
#include "ib/SocketConnector.hpp"
#include "ib/SocketInitiator.hpp"

#include "ib/TestHarness.hpp"
#include "ib/VersionSpecific.hpp"
#include "zmq/Publisher.hpp"
#include "zmq/ZmqUtils.hpp"


using namespace IBAPI;

enum Event {
  // Strategy
  ON_CONNECT,
  ON_ERROR,
  ON_DISCONNECT,
  ON_TIMEOUT,

  // Application
  ON_LOGON,
  ON_LOGOUT,
  TO_ADMIN,
  TO_APP,
  FROM_ADMIN,
  FROM_APP
};

class TestApplication : public IBAPI::ApplicationBase,
                        public TestHarnessBase<Event>
{
 public:

  TestApplication() {}
  ~TestApplication() {}

  void onLogon(const SessionID& sessionId)
  {
    incr(ON_LOGON);
  }

  void onLogout(const SessionID& sessionId)
  {
    incr(ON_LOGOUT);
  }
};

class TestStrategy :
    public IBAPI::StrategyBase,
    public TestHarnessBase<Event>
{
 public :
  TestStrategy() {}
  ~TestStrategy() { }

  void onConnect(SocketConnector& sc, int clientId)
  {
    incr(ON_CONNECT);
  }
};

using ib::internal::EWrapperFactory;


TEST(MarketDataTest, RequestMarketDataTest)
{
  TestApplication app;
  TestStrategy strategy;

  // Service endpoint of the connector -- where commands are received.
  const string& reactorAddr =
      "ipc://_zmq.RequestMarketDataTest.in";

  // Use inproc for socketConnector and the publisher.
  const string& outboundAddr = "inproc://RequestMarketDataTest";
  SocketConnector::ZmqAddressMap outboundChannels;
  outboundChannels[0] = outboundAddr;

  // The market data feed address
  const string& marketDataAddr = "ipc://_zmq.RequestMarketDataTest.pub";

  zmq::context_t sharedContext(1);
  LOG(INFO) << "Context = " << &sharedContext;

  LOG(INFO) << "Creating socket connector ==========================";
  // Connector's outbound address is the publisher's inbound address.
  SocketConnector socketConnector(reactorAddr, outboundChannels, app, 10,
                                  &sharedContext, &sharedContext);

  LOG(INFO) << "Starting publisher =================================";
  atp::zmq::Publisher publisher(outboundAddr, marketDataAddr,
                                &sharedContext);

  int clientId = 1245678;
  int status = socketConnector.connect("127.0.0.1", 4001, clientId,
                                       &strategy);
  EXPECT_EQ(clientId, status); // Expected, actual
  EXPECT_EQ(1, strategy.getCount(ON_CONNECT));

  // Subscriber client
  zmq::context_t consumerCtx(1);
  zmq::socket_t consumer(consumerCtx, ZMQ_SUB);
  consumer.connect(marketDataAddr.c_str());
  consumer.setsockopt(ZMQ_SUBSCRIBE, "", 0);

  // R client
  zmq::context_t rClientCtx(1);
  zmq::socket_t rClient(rClientCtx, ZMQ_REQ);
  rClient.connect(reactorAddr.c_str());

  // Get a version specific market data request
  ib::internal::ApiMessageBase* mdr =
      ib::testing::VersionSpecific::getMarketDataRequestForTest();

  bool sent = mdr->send(rClient);
  EXPECT_TRUE(sent);

  // listen for reply
  std::string reply;
  atp::zmq::receive(rClient, &reply);
  EXPECT_EQ("200", reply);

  // Now consumer receives the data:
  int count = 50; // Just 10 price/size ticks
  while (count--) {
    std::ostringstream oss;
    while (1) {
      std::string buff;
      int more = atp::zmq::receive(consumer, &buff);
      oss << buff << ' ';
      if (more == 0) break;
    }
    LOG(INFO) << "Subscriber: " << oss.str();
  }

  LOG(INFO) << "Finishing." << std::endl;
  socketConnector.stop();

  LOG(INFO) << "Stopped: " << &sharedContext;
}




using IBAPI::SocketInitiator;
using IBAPI::SessionSetting;

TEST(MarketDataTest, SocketInitiatorRequestMarketDataTest)
{
  TestApplication app;

  SocketInitiator::SessionSettings settings;
  SessionSetting setting1(1000, "127.0.0.1", 4001, "ipc://_zmq.connector1.in");
  SessionSetting setting2(1001, "127.0.0.1", 4001, "ipc://_zmq.connector2.in");

  settings.push_back(setting1);
  settings.push_back(setting2);

  SocketInitiator initiator(app, settings);
  initiator.publish(0, "ipc://_zmq.marketdata");
  initiator.publish(1, "ipc://_zmq.account");

  initiator.start();

  app.waitForFirstOccurrence(ON_LOGON, 2);
  EXPECT_EQ(app.getCount(ON_LOGON), 2);

  // Subscriber client
  zmq::context_t consumerCtx(1);
  zmq::socket_t consumer(consumerCtx, ZMQ_SUB);
  consumer.connect("ipc://_zmq.marketdata");
  consumer.setsockopt(ZMQ_SUBSCRIBE, "", 0);

  // R client
  zmq::context_t rClientCtx(1);
  zmq::socket_t rClient(rClientCtx, ZMQ_REQ);
  rClient.connect("ipc://_zmq.connector1.in");

  // Get a version specific market data request
  ib::internal::ApiMessageBase* mdr =
      ib::testing::VersionSpecific::getMarketDataRequestForTest();

  bool sent = mdr->send(rClient);
  EXPECT_TRUE(sent);

  // listen for reply
  std::string reply;
  atp::zmq::receive(rClient, &reply);
  EXPECT_EQ("200", reply);

  // Now consumer receives the data:
  int count = 50; // Just 10 price/size ticks
  while (count--) {
    std::ostringstream oss;
    while (1) {
      std::string buff;
      int more = atp::zmq::receive(consumer, &buff);
      oss << buff << ' ';
      if (more == 0) break;
    }
    LOG(INFO) << "Subscriber: " << oss.str();
  }

  LOG(INFO) << "Finishing." << std::endl;
  initiator.stop();

  LOG(INFO) << "Stopped.";
}

