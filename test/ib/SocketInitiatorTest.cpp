
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
#include "ib/GenericTickRequest.hpp"
#include "ib/AsioEClientSocket.hpp"
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
};


TEST(SocketInitiatorTest, SocketInitiatorStartTest)
{
  TestApplication app;

  const std::string& zmqInbound1 = "tcp://127.0.0.1:15555";
  const std::string& zmqInbound2 = "tcp://127.0.0.1:15556";

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

  //sleep(5);
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

  std::list<SessionSetting> settings;
  settings.push_back(setting1);
  settings.push_back(setting2);
  settings.push_back(setting3);

  LOG(INFO) << "Start the initiator =====================================";

  // Now the initiator that will connect to the gateway.
  TestInitiator initiator(app, settings);

  LOG(INFO) << "Connect to gateway  =====================================";

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
  sleep(5);

  LOG(INFO) << "Now stopping the initiator";
  initiator.stop();

  EXPECT_FALSE(initiator.isLoggedOn());

  LOG(INFO) << "Test finished.";
}
