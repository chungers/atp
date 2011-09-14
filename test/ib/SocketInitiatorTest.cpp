
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

class TestStrategy : public IBAPI::StrategyBase, public TestHarnessBase<Event>
{
 public :
  TestStrategy() {}
  ~TestStrategy() {}

  void onConnect(SocketConnector& sc, int clientId)
  {
    incr(ON_CONNECT);
  }
};


TEST(SocketInitiatorTest, SocketInitiatorStartTest)
{
  TestApplication app;
  IBAPI::SessionSetting setting1(1, "127.0.0.1", 4001);
  IBAPI::SessionSetting setting2(2);

  std::list<SessionSetting> settings;
  settings.push_back(setting1);
  settings.push_back(setting2);

  std::list<SessionSetting>::iterator itr;
  for (itr = settings.begin(); itr != settings.end(); ++itr) {
    LOG(INFO) << "Setting: " << *itr << std::endl;
  }

  IBAPI::SocketInitiator initiator(app, settings);

  initiator.start();

  app.waitForFirstOccurrence(ON_LOGON, 10);

  sleep(10);
}
