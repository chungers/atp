
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
  {}

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
  const std::string& zmqOutbound1 = "tcp://127.0.0.1:17777";
  const std::string& zmqOutbound2 = "tcp://127.0.0.1:17778";

  IBAPI::SessionSetting setting1(1, "127.0.0.1", 4001,
                                 zmqInbound1, zmqOutbound1);
  IBAPI::SessionSetting setting2(2, "127.0.0.1", 4001,
                                 zmqInbound2, zmqOutbound2);

  std::list<SessionSetting> settings;
  settings.push_back(setting1);
  settings.push_back(setting2);

  std::list<SessionSetting>::iterator itr;
  for (itr = settings.begin(); itr != settings.end(); ++itr) {
    LOG(INFO) << "Setting: " << *itr << std::endl;
  }

  TestInitiator initiator(app, settings);

  initiator.start();

  app.waitForNOccurrences(ON_LOGON, 2, 10);
  EXPECT_EQ(app.getCount(ON_LOGON), 2);

  initiator.waitForNOccurrences(ON_CONNECT, 2, 10);
  EXPECT_EQ(initiator.getCount(ON_CONNECT), 2);
}
