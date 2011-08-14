
#include <map>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <gtest/gtest.h>

#include <boost/asio.hpp>

#include "ib/tick_types.hpp"
#include "ib/logging_impl.hpp"
#include "ib/AsioEClientSocket.hpp"
#include "ib/ticker_id.hpp"

using namespace ib::internal;

static const std::string GENERIC_TICK_TAGS =
    "100,101,104,105,106,107,165,221,225,233,236,258";


class TestEWrapper : public LoggingEWrapper {
 public:
  enum EVENT {
    NEXT_VALID_ID = 1,
    TICK_PRICE = 3,
    TICK_SIZE = 5,
    TICK_GENERIC = 7
  };

  TestEWrapper() : LoggingEWrapper() {
    LOG(INFO) << "Initialized LoggingEWrapper." << std:: endl;
  }

 public:

  int getCount(EVENT event) {
    if (eventCount_.find(event) != eventCount_.end()) {
      return eventCount_[event];
    } else {
      return 0;
    }
  }
  
  void nextValidId(OrderId orderId) {
    LoggingEWrapper::nextValidId(orderId);
    incr(NEXT_VALID_ID, 1);
  }

 private:

  void incr(EVENT event, int count) {
    if (eventCount_.find(event) == eventCount_.end()) {
      eventCount_[event] = count;
    } else {
      eventCount_[event] += count;
    }
  }
  
  std::map<EVENT, int> eventCount_;
};

bool waitForConnection(AsioEClientSocket& ec, int attempts) {
  int tries = 0;
  for (int tries = 0; !ec.isConnected() && tries < attempts; ++tries) {
    LOG(INFO) << "Waiting for connection setup." << std::endl;
    sleep(1);
  }
  return ec.isConnected();
}



/**
   Basic connection test.
 */
TEST(AsioEClientSocketTest, ConnectionTest)
{
  boost::asio::io_service ioService;
  TestEWrapper ew;
  AsioEClientSocket ec(ioService, ew);

  LOG(INFO) << "Started " << ioService.run() << std::endl;
  EXPECT_TRUE(ec.eConnect("127.0.0.1", 4001, 0));

  bool connected = waitForConnection(ec, 5);
  EXPECT_TRUE(connected);

  // Disconnect
  ec.eDisconnect();

  EXPECT_FALSE(ec.isConnected());

  // Check invocation of the nextValidId -- this is part of the
  // TWS API protocol.
  EXPECT_EQ(1, ew.getCount(TestEWrapper::NEXT_VALID_ID));
}


/**
   Request basic market data.
 */
TEST(AsioEClientSocketTest, RequestMarketDataTest)
{
  boost::asio::io_service ioService;
  TestEWrapper ew;
  AsioEClientSocket ec(ioService, ew);

  LOG(INFO) << "Started " << ioService.run() << std::endl;
  EXPECT_TRUE(ec.eConnect("127.0.0.1", 4001, 0));

  bool connected = waitForConnection(ec, 5);
  EXPECT_TRUE(connected);

  // Request market data:
  Contract c;
  c.symbol = "AAPL";
  c.secType = "STK";
  c.exchange = "SMART";
  c.currency = "USD";
  TickerId tid = SymbolToTickerId("AAPL");

  LOG(INFO) << "Requesting market data for " << c.symbol << " with id = "
            << tid << std::endl;
  
  ec.reqMktData(tid, c, GENERIC_TICK_TAGS, false);

  LOG(INFO) << "Requesting market depth." << std::endl;
  
  //ec.reqMktDepth(tid, c, 10);

  while (true) sleep(600);
  // Disconnect
  ec.eDisconnect();

  EXPECT_FALSE(ec.isConnected());

  // Check invocation of the nextValidId -- this is part of the
  // TWS API protocol.
  EXPECT_EQ(1, ew.getCount(TestEWrapper::NEXT_VALID_ID));
}

