
#include <algorithm>
#include <map>
#include <list>
#include <vector>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <gtest/gtest.h>

#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/shared_ptr.hpp>

#include "ib/Application.hpp"
#include "ib/ApplicationBase.hpp"
#include "ib/GenericTickRequest.hpp"
#include "ib/EWrapperFactory.hpp"
#include "ib/SessionSetting.hpp"
#include "ib/SocketConnector.hpp"
#include "ib/SocketInitiator.hpp"

#include "ib/TestHarness.hpp"
#include "ib/ticker_id.hpp"
#include "zmq/Publisher.hpp"


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

class TestInitiator : public IBAPI::SocketInitiator,
                      public TestHarnessBase<Event>
{
 public :
  TestInitiator(Application& app, std::list<SessionSetting>& settings) :
      IBAPI::SocketInitiator(app, settings)
  {
  }

  void onConnect(SocketConnector& sc, int clientId)
  {
    LOG(INFO) << "****** CONNECTED " << clientId;
    incr(ON_CONNECT);
  }

  void onDisconnect(SocketConnector& sc, int clientId)
  {
    LOG(INFO) << "****** DISCONNECTED " << clientId;
    incr(ON_DISCONNECT);
  }
};


TEST(SocketInitiatorTest, SocketInitiatorStartTest)
{
  TestApplication app;

  const std::string& zmqInbound1 = "tcp://127.0.0.1:35555";
  const std::string& zmqInbound2 = "tcp://127.0.0.1:35556";

  IBAPI::SessionSetting setting1(1, "127.0.0.1", 4001, zmqInbound1);
  IBAPI::SessionSetting setting2(2, "127.0.0.1", 4001, zmqInbound2);

  std::list<SessionSetting> settings;
  settings.push_back(setting1);
  settings.push_back(setting2);

  std::list<SessionSetting>::iterator itr;
  for (itr = settings.begin(); itr != settings.end(); ++itr) {
    LOG(INFO) << "Setting: " << *itr << std::endl;
  }

  TestInitiator initiator(app, settings);

  initiator.start();

  LOG(INFO) << "Waiting for LOGON events.";

  app.waitForNOccurrences(ON_LOGON, 2, 5);
  EXPECT_EQ(app.getCount(ON_LOGON), 2);

  LOG(INFO) << "Waiting for CONNECT events.";
  initiator.waitForNOccurrences(ON_CONNECT, 2, 5);
  EXPECT_EQ(initiator.getCount(ON_CONNECT), 2);

  initiator.stop();
  LOG(INFO) << "Test finished.";
}


static void blockInSeparateThread(SocketInitiator* initiator)
{
  boost::thread th(boost::bind(&SocketInitiator::block, initiator));
  LOG(INFO) << "Blocking in separate thread.";
}

TEST(SocketInitiatorTest, SocketInitiatorEmbeddedPublisherTest)
{
  TestApplication app;

  const std::string& zmqInbound1 = "tcp://127.0.0.1:25555";
  const std::string& zmqInbound2 = "tcp://127.0.0.1:25556";
  const std::string& zmqInbound3 = "tcp://127.0.0.1:25557";

  IBAPI::SessionSetting setting1(10, "127.0.0.1", 4001, zmqInbound1);
  IBAPI::SessionSetting setting2(20, "127.0.0.1", 4001, zmqInbound2);
  IBAPI::SessionSetting setting3(30, "127.0.0.1", 4001, zmqInbound3);

  SocketInitiator::SessionSettings settings;
  settings.push_back(setting1);
  settings.push_back(setting2);
  settings.push_back(setting3);

  LOG(INFO) << "Start the initiator =====================================";

  // Now the initiator that will connect to the gateway.
  TestInitiator initiator(app, settings);

  // Set up publishers
  LOG(INFO) << "Starting publishers  =====================================";
  bool publisherStarted = true;
  const std::string publish = "tcp://127.0.0.1:17777";
  initiator.publish(0, publish);
  initiator.publish(1, publish);

  LOG(INFO) << "Connect to gateway  =========================+============";

  initiator.start();

  LOG(INFO) << "Waiting for LOGON events.";

  app.waitForNOccurrences(ON_LOGON, 3, 5);
  EXPECT_EQ(app.getCount(ON_LOGON), 3);

  LOG(INFO) << "Waiting for CONNECT events.";
  initiator.waitForNOccurrences(ON_CONNECT, 3, 5);
  EXPECT_EQ(initiator.getCount(ON_CONNECT), 3);

  // This tests that the block method on the initiator properly blocks
  // synchronously until stop() on the initiator is called from another
  // thread and properly shuts everything donw.
  blockInSeparateThread(&initiator);

  EXPECT_TRUE(initiator.isLoggedOn());

  LOG(INFO) << "Sleep for a bit";
  sleep(3);

  // Try to connect to the publishing endpoint.
  if (publisherStarted) {
    zmq::context_t c(1);
    zmq::socket_t subscriber(c, ZMQ_SUB);
    subscriber.connect(publish.c_str());
    LOG(INFO) << "Subscriber connected to " << publish;
  }

  // Try calling publish / push methods.  We should
  // see exceptions since the connectors are already running
  // and we forbid calling these methods after start() is called.
  try {
    initiator.publish(0, "tcp://127.0.0.1:14444");
    FAIL();
  } catch (IBAPI::RuntimeError r) {
    LOG(INFO) << "Runtime error: " << r.what();
  }

  try {
    initiator.push(0, "tcp://127.0.0.1:14444");
    FAIL();
  } catch (IBAPI::RuntimeError r) {
    LOG(INFO) << "Runtime error: " << r.what();
  }

  LOG(INFO) << "Now stopping the initiator";
  initiator.stop();

  LOG(INFO) << "Waiting for DISCONNECT events.";
  initiator.waitForNOccurrences(ON_DISCONNECT, 3, 5);
  EXPECT_EQ(initiator.getCount(ON_DISCONNECT), 3);

  EXPECT_FALSE(initiator.isLoggedOn());

  LOG(INFO) << "Test finished.";
}


TEST(SocketInitiatorTest, SocketInitiatorPushExternalTest)
{
  TestApplication app;

  const std::string& zmqInbound1 = "tcp://127.0.0.1:15555";
  const std::string& zmqInbound2 = "tcp://127.0.0.1:15556";

  IBAPI::SessionSetting setting1(11, "127.0.0.1", 4001, zmqInbound1);
  IBAPI::SessionSetting setting2(21, "127.0.0.1", 4001, zmqInbound2);

  SocketInitiator::SessionSettings settings;
  settings.push_back(setting1);
  settings.push_back(setting2);

  LOG(INFO) << "Start the initiator =====================================";

  // Now the initiator that will connect to the gateway.
  TestInitiator initiator(app, settings);

  // Set up publishers
  LOG(INFO) << "Starting publishers  =====================================";
  bool publisherStarted = true;
  const std::string pushTo = "tcp://127.0.0.1:17776";
  const std::string publish = "tcp://127.0.0.1:17777";

  initiator.push(0, pushTo);
  initiator.push(1, pushTo);

  LOG(INFO) << "Connect to gateway  ======================================";

  initiator.start();

  LOG(INFO) << "Waiting for LOGON events.";

  app.waitForNOccurrences(ON_LOGON, 2, 5);

  EXPECT_TRUE(initiator.isLoggedOn());
  EXPECT_EQ(app.getCount(ON_LOGON), 2);

  LOG(INFO) << "Sleep for a bit";
  sleep(3);

  // Try to connect to the publishing endpoint.
  if (publisherStarted) {
    zmq::context_t c(1);
    zmq::socket_t subscriber(c, ZMQ_SUB);
    subscriber.connect(publish.c_str());
    LOG(INFO) << "Subscriber connected to " << publish;
  }

  LOG(INFO) << "Now stopping the initiator";
  initiator.stop();

  LOG(INFO) << "Waiting for DISCONNECT events.";
  initiator.waitForNOccurrences(ON_DISCONNECT, 2, 5);
  EXPECT_EQ(initiator.getCount(ON_DISCONNECT), 2);

  EXPECT_FALSE(initiator.isLoggedOn());

  LOG(INFO) << "Test finished.";
}
