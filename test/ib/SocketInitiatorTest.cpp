
#include <algorithm>
#include <map>
#include <set>
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
#include "ib/SocketConnector.hpp"

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

class TestApplication : public IBAPI::ApplicationBase, public TestHarnessBase<Event>
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

/// Basic test for establishing connections.
TEST(SocketInitiatorTest, SocketConnectorTest)
{
  TestApplication app;
  TestStrategy strategy;

  SocketConnector socketConnector(app, 10);

  int clientId = 1;
  int status = socketConnector.connect("127.0.0.1", 4001, clientId,
                                       &strategy);
  
  EXPECT_EQ(status, clientId);
  EXPECT_EQ(strategy.getCount(ON_CONNECT), 1);

  app.waitForFirstOccurrence(ON_LOGON, 10);

  EXPECT_EQ(app.getCount(ON_LOGON), 1);
}

