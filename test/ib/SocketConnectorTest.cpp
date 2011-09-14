
#include <algorithm>
#include <iostream>
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

/// Basic test for instantiation and destroy
TEST(SocketConnectorTest, InstantiationTest)
{
  TestApplication app;
  TestStrategy strategy;

  SocketConnector socketConnector(app, 10);

  // Client
  zmq::context_t context(1);
  zmq::socket_t client(context, ZMQ_REQ);
  client.connect("ipc://connector1");

  size_t messages = 5000;
  for (unsigned int i = 0; i < messages; ++i) {
    std::ostringstream oss;
    oss << "Message-" << i;

    std::string message(oss.str());
    bool exception = false;
    try {
      atp::zmq::send_zero_copy(client, message);

      std::string reply;
      atp::zmq::receive(client, &reply);

      ASSERT_EQ(message, reply);
    } catch (zmq::error_t e) {
      LOG(ERROR) << "Exception: " << e.what() << std::endl;
      exception = true;
    }
    ASSERT_FALSE(exception);
  }
}

TEST(SocketConnectorTest, DISABLED_ConnectionTest)
{
  TestApplication app;
  TestStrategy strategy;

  SocketConnector socketConnector(app, 10);

  sleep(5);

  int clientId = 1;
  int status = socketConnector.connect("127.0.0.1", 4001, clientId,
                                       &strategy);

  EXPECT_EQ(status, clientId);
  EXPECT_EQ(strategy.getCount(ON_CONNECT), 1);

  LOG(INFO) << "Checked on_connect = " << strategy.getCount(ON_CONNECT)
            << std::endl;

  app.waitForFirstOccurrence(ON_LOGON, 10);

  EXPECT_EQ(app.getCount(ON_LOGON), 1);
}
