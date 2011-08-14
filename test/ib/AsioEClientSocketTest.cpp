
#include <algorithm>
#include <map>
#include <set>
#include <vector>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <gtest/gtest.h>

#include <boost/asio.hpp>
#include <boost/format.hpp>

#include "ib/GenericTickRequest.hpp"
#include "ib/logging_impl.hpp"
#include "ib/AsioEClientSocket.hpp"
#include "ib/ticker_id.hpp"

using namespace ib::internal;


static std::string FormatOptionExpiry(int year, int month, int day)
{
  ostringstream s1;
  string fmt = (month > 9) ? "%4d%2d" : "%4d0%1d";
  string fmt2 = (day > 9) ? "%2d" : "0%1d";
  s1 << boost::format(fmt) % year % month << boost::format(fmt2) % day;
  return s1.str();
}


// EWrapper implementation for test -- tracks invocations and ticker id seen.
class TestEWrapper : public LoggingEWrapper {
 public:
  enum EVENT {
    NEXT_VALID_ID = 1,
    TICK_PRICE,
    TICK_SIZE,
    TICK_GENERIC,
    TICK_OPTION_COMPUTATION,
    UPDATE_MKT_DEPTH,
    CONTRACT_DETAILS,
    CONTRACT_DETAILS_END // for option chain
  };

  TestEWrapper() : LoggingEWrapper(),
                   optionChain_(NULL) {
    LOG(INFO) << "Initialized LoggingEWrapper." << std:: endl;
  }

 public:

  // Returns the count of an event.
  int getCount(EVENT event) {
    if (eventCount_.find(event) != eventCount_.end()) {
      return eventCount_[event];
    } else {
      return 0;
    }
  }

  // Returns true if the ticker id was seen.
  bool hasSeenTickerId(TickerId id) {
    return tickerIds_.find(id) != tickerIds_.end();
  }
  
  // @Override
  void nextValidId(OrderId orderId) {
    LoggingEWrapper::nextValidId(orderId);
    incr(NEXT_VALID_ID);
  }

  // @Override
  void tickPrice(TickerId tickerId, TickType tickType, double price, int canAutoExecute) {
    LoggingEWrapper::tickPrice(tickerId, tickType, price, canAutoExecute);
    incr(TICK_PRICE);
    seen(tickerId);
  }

  // @Override
  void tickSize(TickerId tickerId, TickType tickType, int size) {
    LoggingEWrapper::tickSize(tickerId, tickType, size);
    incr(TICK_SIZE);
    seen(tickerId);
  }

  // @Override
  void tickGeneric(TickerId tickerId, TickType tickType, double value) {
    LoggingEWrapper::tickGeneric(tickerId, tickType, value);
    incr(TICK_GENERIC);
    seen(tickerId);
  }

  // @Override
  void tickOptionComputation(TickerId tickerId, TickType tickType, double impliedVol, double delta,
                             double optPrice, double pvDividend,
                             double gamma, double vega, double theta, double undPrice) {
    LoggingEWrapper::tickOptionComputation(tickerId, tickType, impliedVol, delta,
                                           optPrice, pvDividend, gamma, vega, theta, undPrice);
    incr(TICK_OPTION_COMPUTATION);
    seen(tickerId);
  }

  // @Override
  void updateMktDepth(TickerId id, int position, int operation, int side,
                      double price, int size) {
    LoggingEWrapper::updateMktDepth(id, position, operation, side, price, size);
    incr(UPDATE_MKT_DEPTH);
    seen(id);
   }

  // @Override
  void contractDetails( int reqId, const ContractDetails& contractDetails) {
    LoggingEWrapper::contractDetails(reqId, contractDetails);
    incr(CONTRACT_DETAILS);
    seen(reqId);
    if (optionChain_) {
      Contract c = contractDetails.summary;
      optionChain_->push_back(c);
    }
  }

  // @Override
  void contractDetailsEnd( int reqId) {
    LoggingEWrapper::contractDetailsEnd(reqId);
    incr(CONTRACT_DETAILS_END);
    seen(reqId);
  }

  void setOptionChain(std::vector<Contract>* optionChain) {
    optionChain_ = optionChain;
  }
  
 private:

  void incr(EVENT event, int count) {
    if (eventCount_.find(event) == eventCount_.end()) {
      eventCount_[event] = count;
    } else {
      eventCount_[event] += count;
    }
  }

  inline void incr(EVENT event) {
    incr(event, 1);
  }

  void seen(TickerId tickerId) {
    if (!hasSeenTickerId(tickerId)) {
      tickerIds_.insert(tickerId);
    }
  }
  
  std::map<EVENT, int> eventCount_;
  std::set<TickerId> tickerIds_;
  std::vector<Contract>* optionChain_;
};


// Spin around until connection is made.
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

  GenericTickRequest genericTickRequest;
  genericTickRequest
      .add(RTVOLUME)
      .add(OPTION_VOLUME)
      .add(OPTION_IMPLIED_VOLATILITY)
      .add(HISTORICAL_VOLATILITY)
      .add(REALTIME_HISTORICAL_VOLATILITY)
      ;

  
  LOG(INFO) << "Request generic ticks: " << genericTickRequest.toString() << std::endl;

  bool snapShot = false;
  ec.reqMktData(tid, c, genericTickRequest.toString(), snapShot);

  // Spin and wait for data.
  int MAX_WAIT = 3; // seconds
  for (int i = 0; i < MAX_WAIT && ew.getCount(TestEWrapper::TICK_PRICE) == 0; ++i) {
    sleep(1);
  }

  // Disconnect
  ec.eDisconnect();

  EXPECT_FALSE(ec.isConnected());

  EXPECT_GT(ew.getCount(TestEWrapper::TICK_PRICE), 1);
  EXPECT_GT(ew.getCount(TestEWrapper::TICK_SIZE), 1);
  EXPECT_TRUE(ew.hasSeenTickerId(tid));
}

/**
   Request index
 */
TEST(AsioEClientSocketTest, RequestIndexMarketDataTest)
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
  c.symbol = "SPX";
  c.secType = "IND";
  c.exchange = "CBOE";
  c.primaryExchange = "CBOE";
  c.currency = "USD";
  TickerId tid = SymbolToTickerId("SPX");

  LOG(INFO) << "Requesting market data for " << c.symbol << " with id = "
            << tid << std::endl;

  GenericTickRequest genericTickRequest;
  genericTickRequest
      .add(RTVOLUME)
      .add(HISTORICAL_VOLATILITY)
      .add(REALTIME_HISTORICAL_VOLATILITY)
      ;

  
  LOG(INFO) << "Request generic ticks: " << genericTickRequest.toString() << std::endl;

  bool snapShot = false;
  ec.reqMktData(tid, c, genericTickRequest.toString(), snapShot);

  // Spin and wait for data.
  int MAX_WAIT = 3; // seconds
  for (int i = 0; i < MAX_WAIT && ew.getCount(TestEWrapper::TICK_PRICE) == 0; ++i) {
    sleep(1);
  }

  // Disconnect
  ec.eDisconnect();

  EXPECT_FALSE(ec.isConnected());

  EXPECT_GT(ew.getCount(TestEWrapper::TICK_PRICE), 1);
  EXPECT_GT(ew.getCount(TestEWrapper::TICK_SIZE), 1);
  EXPECT_TRUE(ew.hasSeenTickerId(tid));
}

/**
   Request market depth
 */
TEST(AsioEClientSocketTest, RequestMarketDepthTest)
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

  LOG(INFO) << "Requesting market depth for " << c.symbol << " with id = "
            << tid << std::endl;

  ec.reqMktDepth(tid, c, 10);

  // Spin and wait for data.
  int MAX_WAIT = 3; // seconds
  for (int i = 0; i < MAX_WAIT && ew.getCount(TestEWrapper::UPDATE_MKT_DEPTH) == 0; ++i) {
    sleep(1);
  }
  

  // Disconnect
  ec.eDisconnect();

  EXPECT_FALSE(ec.isConnected());

  EXPECT_EQ(ew.getCount(TestEWrapper::NEXT_VALID_ID), 1);
  EXPECT_GT(ew.getCount(TestEWrapper::UPDATE_MKT_DEPTH), 1);
  EXPECT_TRUE(ew.hasSeenTickerId(tid));
}


struct SortByStrike {
  bool operator()(const Contract& a, const Contract& b) {
    return a.strike < b.strike;
  }
} sortByStrikeFunctor;


/**
   Request contract information / option chain
 */
TEST(AsioEClientSocketTest, RequestOptionChainTest)
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
  c.secType = "OPT";
  c.exchange = "SMART";
  c.currency = "USD";
  c.expiry = FormatOptionExpiry(2011, 8, 19);
  c.right = "C"; // call - P for put
  
  int requestId = 1000; // Not a ticker id.  This is just a request id.

  LOG(INFO) << "Requesting option chain for " << c.symbol << " with request id = "
            << requestId << std::endl;

  // Collect the option chain
  std::vector<Contract> optionChain;
  ew.setOptionChain(&optionChain);
  
  ec.reqContractDetails(requestId, c);

  // Spin and wait for data.
  int MAX_WAIT = 5; // seconds
  for (int i = 0; i < MAX_WAIT && ew.getCount(TestEWrapper::CONTRACT_DETAILS_END) == 0; ++i) {
    sleep(1);
  }
  
  EXPECT_EQ(ew.getCount(TestEWrapper::CONTRACT_DETAILS_END), 1);
  EXPECT_TRUE(ew.hasSeenTickerId(requestId));

  // Check the option chain
  EXPECT_GT(optionChain.size(), 0);

  LOG(INFO) << "Received option chain." << std::endl;
  
  // Sort the option chain by strike
  std::sort(optionChain.begin(), optionChain.end(), sortByStrikeFunctor);
  
  GenericTickRequest genericTickRequest;  // note we are setting any special requests.
  
  // Iterate through the option chain and make market data request:
  std::vector<int> tids;
  int tid = 10000;
  for (std::vector<Contract>::iterator itr = optionChain.begin(); itr != optionChain.end(); ++itr) {
    LOG(INFO) << "Requesting market data for contract / " << itr->localSymbol << std::endl;
    ec.reqMktData(++tid, *itr, genericTickRequest.toString(), false);
    tids.push_back(tid);
  }

  // Wait until all the data has come through for all the contracts.
  for (std::vector<int>::iterator itr = tids.begin(); itr != tids.end(); ++itr) {
    int MAX_WAIT = 3; 
    for (int i = 0; i < MAX_WAIT && !ew.hasSeenTickerId(*itr); ++i) {
      sleep(1);
    }
  }

  // Disconnect
  ec.eDisconnect();
  EXPECT_FALSE(ec.isConnected());

  EXPECT_GT(ew.getCount(TestEWrapper::TICK_OPTION_COMPUTATION), 1);

  // Checks that we have seen the ticker ids for all the option contracts.
  for (std::vector<int>::iterator itr = tids.begin(); itr != tids.end(); ++itr) {
    EXPECT_TRUE(ew.hasSeenTickerId(*itr));
  }
}

