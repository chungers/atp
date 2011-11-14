
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

#include "ib/internal.hpp"
#include "ib/Application.hpp"
#include "ib/GenericTickRequest.hpp"
#include "ib/AsioEClientSocket.hpp"
#include "ib/EWrapperFactory.hpp"
#include "ib/SessionSetting.hpp"
#include "ib/SocketConnector.hpp"
#include "ib/AbstractSocketConnector.hpp"
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

class TestSocketConnector : public ib::internal::AbstractSocketConnector
{
 public:
  TestSocketConnector(const string& responderAddress,
                      const string& outboundAddr,
                      Application& app, int timeout,
                      zmq::context_t* context = NULL) :
      AbstractSocketConnector(responderAddress, outboundAddr, app, timeout,
                              context)
  {
    LOG(INFO) << "TestConnector initialized, context = " << context;
    LOG(INFO) << "Inbound @ " << responderAddress;
    LOG(INFO) << "Outbound @ " << outboundAddr;
  }

 protected:

  bool handleReactorInboundMessages(
      zmq::socket_t& reactorSocket, ib::internal::EClientPtr eclient)
  {
    // Make sure that the eclient pointer is not null
    EClient* ptr = eclient.get();
    EXPECT_TRUE(ptr != NULL);

    zmq::socket_t* outboundSocket = getOutboundSocket();

    // This is run in a thread separate from the AsioEClientSocket's
    // block() thread.  That thread has access to the publish socket.
    // We need to make sure that no other thread has access to the socket.
    EXPECT_EQ(NULL, outboundSocket);

    int more = atp::zmq::receive(reactorSocket, &msg);
    LOG(INFO) << "Received " << msg << std::endl;

    try {
      // just echo back to the client
      size_t sent = atp::zmq::send_zero_copy(reactorSocket, msg);
      return sent > 0;

    } catch (zmq::error_t e) {
      LOG(ERROR) << "Exception " << e.what() << std::endl;
      return false;
    }
  }

 private:
  std::string msg;
};


TEST(SocketConnectorTest, AbstractSocketConnectorConnectionTest)
{
  TestApplication app;
  TestStrategy strategy;

  const string& bindAddr =
      "ipc://_zmq.AbstractSocketConnectorConnectionTest.in";
  const string& outboundAddr ="tcp://127.0.0.1:5555";

  TestSocketConnector socketConnector(bindAddr, outboundAddr, app, 10);

  int clientId = 14567;
  int status = socketConnector.connect("127.0.0.1", 4001, clientId,
                                       &strategy);

  EXPECT_EQ(clientId, status); // Expected, actual
  EXPECT_EQ(1, strategy.getCount(ON_CONNECT));

  LOG(INFO) << "Checked on_connect = " << strategy.getCount(ON_CONNECT)
            << std::endl;

  app.waitForFirstOccurrence(ON_LOGON, 10);

  EXPECT_EQ(app.getCount(ON_LOGON), 1);

  LOG(INFO) << "Test complete." << std::endl;
  socketConnector.stop();
}


/// Basic test for instantiation and destroy
TEST(SocketConnectorTest, SendMessageTest)
{
  TestApplication app;
  TestStrategy strategy;

  const string& bindAddr =
      "ipc://_zmq.AbstractSocketConnectorTest.in";

  // In this case, the outbound socket for the connector is simply
  // a domain socket file for IPC.  See above test case for actually
  // connecting to a hub over tcp.
  const string& outboundAddr =
      "ipc://_zmq.AbstractSocketConnectorTest.out";

  zmq::context_t context(1);
  TestSocketConnector socketConnector(bindAddr, outboundAddr, app, 10);


  LOG(INFO) << "Starting client."  << std::endl;

  // Client
  zmq::socket_t client(context, ZMQ_REQ);
  client.connect(bindAddr.c_str());

  LOG(INFO) << "Client connected."  << std::endl;

  int clientId = 12456;
  int status = socketConnector.connect("127.0.0.1", 4001, clientId,
                                       &strategy);
  EXPECT_EQ(clientId, status); // Expected, actual
  EXPECT_EQ(1, strategy.getCount(ON_CONNECT));

  size_t messages = 10;
  for (unsigned int i = 0; i < messages; ++i) {
    std::ostringstream oss;
    oss << "Message-" << i;

    std::string message(oss.str());
    bool exception = false;
    try {
      VLOG(40) << "sending " << message << std::endl;

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

  LOG(INFO) << "Finishing." << std::endl;
  socketConnector.stop();
}

