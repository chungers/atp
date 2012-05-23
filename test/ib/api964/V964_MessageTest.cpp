#include <sstream>
#include <vector>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <glog/logging.h>
#include <gtest/gtest.h>
#include <ql/quantlib.hpp>
#include <zmq.hpp>
#include <stdlib.h>

#include "utils.hpp"
#include "common.hpp"

#include "ib/internal.hpp"
#include "ib/AsioEClientSocket.hpp"
#include "ib/EWrapperFactory.hpp"
#include "ib/TestHarness.hpp"
#include "ib/TickerMap.hpp"

#include "ApiMessages.hpp"
#include "zmq/Reactor.hpp"


using FIX::FieldMap;


using namespace ib::internal;
using namespace IBAPI;
using namespace QuantLib;
using QuantLib::Date;
using IBAPI::V964::MarketDataRequest;
using IBAPI::V964::OptionChainRequest;


static void print(const FIX::FieldMap& request)
{
  for (FIX::FieldMap::iterator itr = request.begin();
       itr != request.end();
       ++itr) {
    LOG(INFO)
        << "{" << itr->second.getValue() << "," <<
        itr->second.getLength() << "/" << itr->second.getTotal()
        << "}" <<
        "[" << itr->second.getField() << ":"
        << "\"" << itr->second.getString() << "\"]" ;
  }
}

TEST(V964_MessageTest, ApiTest)
{
  LOG(INFO) << "Current TimeMicros = " << now_micros() ;

  MarketDataRequest request;

  // Using set(X) as the type-safe way (instead of setField())
  request.set(FIX::SecurityType(FIX::SecurityType_OPTION));
  request.set(FIX::PutOrCall(FIX::PutOrCall_PUT));

  FIX::Symbol aapl("AAPL");
  request.set(aapl);
  request.set(FIX::DerivativeSecurityID("AAPL 20111119C00450000"));
  request.set(FIX::SecurityExchange(IBAPI::SecurityExchange_SMART));
  request.set(FIX::StrikePrice(425.50));  // Not a valid strike but for testing
  request.set(FIX::MaturityMonthYear("201111"));
  request.set(FIX::MaturityDay("19"));
  request.set(FIX::ContractMultiplier(100));
  request.set(FIX::MDEntryRefID("123456"));

  print(request.getHeader());
  print(request);
  print(request.getTrailer());

  FIX::PutOrCall putOrCall;
  request.get(putOrCall);
  EXPECT_EQ(FIX::PutOrCall_PUT, putOrCall);

  FIX::StrikePrice strike;
  request.get(strike);
  EXPECT_EQ("425.5", strike.getString());
  EXPECT_EQ(425.5, strike);

  FIX::MaturityMonthYear expiryMonthYear;
  request.get(expiryMonthYear);

  FIX::MaturityDay expiryDay;
  request.get(expiryDay);

  const std::string& expiry =
      expiryMonthYear.getString() + expiryDay.getString();
  EXPECT_EQ("20111119", expiry);

  // Get header information
  const ib::internal::Header& header = request.getHeader();

  FIX::BeginString msgMatchKey; // First field, for zmq subscription matching.
  header.get(msgMatchKey);
  EXPECT_EQ("V964.MarketDataRequest", msgMatchKey.getString());

  FIX::MsgType msgType;
  header.get(msgType);
  EXPECT_EQ("MarketDataRequest", msgType.getString());

  const ib::internal::Trailer& trailer = request.getTrailer();
  IBAPI::Ext_SendingTimeMicros sendingTimeMicros;
  trailer.get(sendingTimeMicros);

  LOG(INFO) << "Timestamp = " << sendingTimeMicros.getString() ;

  // Test the actual Contract marshalling:
  Contract c;
  request.marshall(c);

  EXPECT_EQ("AAPL", c.symbol);
  EXPECT_EQ("P", c.right);
  EXPECT_EQ("20111119", c.expiry);
  EXPECT_EQ(425.5, c.strike);
  EXPECT_EQ("USD", c.currency);
  EXPECT_EQ("SMART", c.exchange);
  EXPECT_EQ("100", c.multiplier);
  EXPECT_EQ("AAPL 20111119C00450000", c.localSymbol);
  EXPECT_EQ(123456, c.conId);
  EXPECT_EQ("USD", c.currency);

  // Test by copy semantics
  MarketDataRequest copyRequest(request);

  Contract c2;
  copyRequest.marshall(c2);

  EXPECT_EQ("AAPL", c2.symbol);
  EXPECT_EQ("P", c2.right);
  EXPECT_EQ("20111119", c2.expiry);
  EXPECT_EQ(425.5, c2.strike);
  EXPECT_EQ("USD", c2.currency);
  EXPECT_EQ("SMART", c2.exchange);
  EXPECT_EQ("100", c2.multiplier);
  EXPECT_EQ("AAPL 20111119C00450000", c2.localSymbol);
  EXPECT_EQ(123456, c2.conId);
  EXPECT_EQ("USD", c2.currency);
}

using atp::zmq::Reactor;
using ib::internal::FIXMessage;

struct ReceiveOneMessage : Reactor::Strategy
{
  ReceiveOneMessage() : done(false) {}

  int socketType() { return ZMQ_PULL; }
  bool respond(zmq::socket_t& socket)
  {
    LOG(INFO) << "Starting to receive.";
    done = zmqMessage.receive(socket);
    LOG(INFO) << "Done receive: " << done;
    return done;
  }
  FIXMessage zmqMessage;
  bool done;
};

static std::string FormatOptionExpiry(int year, int month, int day)
{
  std::ostringstream s1;
  std::string fmt = (month > 9) ? "%4d%2d" : "%4d0%1d";
  std::string fmt2 = (day > 9) ? "%2d" : "0%1d";
  s1 << boost::format(fmt) % year % month << boost::format(fmt2) % day;
  return s1.str();
}

TEST(V964_MessageTest, ZmqSendTest)
{
  // Set up the reactor
  const std::string& addr =
      "ipc://_zmq.V964_MessageTest_zmqSendTest.in";
  ReceiveOneMessage strategy;
  Reactor reactor(addr, strategy);

  LOG(INFO) << "Client connecting.";

  // Now set up client to send
  zmq::context_t context(1);
  zmq::socket_t client(context, ZMQ_PUSH);
  client.connect(addr.c_str());

  LOG(INFO) << "Client connected.";

  MarketDataRequest request;

  // Using set(X) as the type-safe way (instead of setField())
  request.set(FIX::Symbol("AAPL"));
  request.set(FIX::SecurityType(FIX::SecurityType_OPTION));
  request.set(FIX::PutOrCall(FIX::PutOrCall_PUT));
  request.set(FIX::Symbol("AAPL"));
  request.set(FIX::DerivativeSecurityID("AAPL 20111119C00450000"));
  request.set(FIX::SecurityExchange(IBAPI::SecurityExchange_SMART));
  request.set(FIX::StrikePrice(450.));
  request.set(FIX::ContractMultiplier(100));
  request.set(FIX::MDEntryRefID("654321"));

  Date today = Date::todaysDate();
  Date nextFriday = Date::nextWeekday(today, QuantLib::Friday);

  IBAPI::V964::FormatExpiry(request, nextFriday);

  std::string dateString = FormatOptionExpiry(
      nextFriday.year(), nextFriday.month(), nextFriday.dayOfMonth());

  LOG(INFO) << "Next Friday = " << nextFriday << ", " << dateString;

  request.send(client);

  LOG(INFO) << "Message sent";

  while (!strategy.done) { sleep(1); }

  EXPECT_FALSE(strategy.zmqMessage.isEmpty());
  EXPECT_EQ(request.totalFields(), strategy.zmqMessage.totalFields());

  MarketDataRequest received(strategy.zmqMessage);

  LOG(INFO) << "Frame by frame compare";

  FIX::FieldMap::iterator itr1 = request.begin();
  FIX::FieldMap::iterator itr2 = received.begin();
  for (; itr1 != request.end(); ++itr1, ++itr2) {

    int comp = strcmp(itr1->second.getValue().c_str(),
                       itr2->second.getValue().c_str());

    EXPECT_EQ(0, comp);

    EXPECT_EQ(itr1->second.getValue(),
              itr2->second.getValue());

    EXPECT_EQ(itr1->second.getValue().size(), itr2->second.getValue().size());
  }

  LOG(INFO) << "Checking message sent.";

  Contract c1;
  request.marshall(c1);

  EXPECT_EQ("AAPL", c1.symbol);
  EXPECT_EQ("OPT", c1.secType);
  EXPECT_EQ("P", c1.right);
  EXPECT_EQ(dateString, c1.expiry);
  EXPECT_EQ(450., c1.strike);
  EXPECT_EQ("USD", c1.currency);
  EXPECT_EQ("SMART", c1.exchange);
  EXPECT_EQ("100", c1.multiplier);
  EXPECT_EQ("AAPL 20111119C00450000", c1.localSymbol);
  EXPECT_EQ(654321, c1.conId);

  LOG(INFO) << "Checking received message.";

  Contract c2;
  received.marshall(c2);

  EXPECT_EQ(c1.symbol, c2.symbol);
  EXPECT_EQ(c1.secType, c2.secType);
  EXPECT_EQ(c1.right, c2.right);
  EXPECT_EQ(c1.expiry, c2.expiry);
  EXPECT_EQ(c1.strike, c2.strike);
  EXPECT_EQ(c1.currency, c2.currency);
  EXPECT_EQ(c1.exchange, c2.exchange);
  EXPECT_EQ(c1.multiplier, c2.multiplier);
  EXPECT_EQ(c1.localSymbol, c2.localSymbol);
  EXPECT_EQ(c1.conId, c2.conId);


  LOG(INFO) << "Test finished.";
}



/// How many seconds max to wait for certain data before timing out.
const int MAX_WAIT = 20;

// Spin around until connection is made.
bool waitForConnection(ib::internal::IBClient& ec, int attempts) {
  int tries = 0;
  for (int tries = 0; !ec.isConnected() && tries < attempts; ++tries) {
    LOG(INFO) << "Waiting for connection setup." << std::endl;
    sleep(1);
  }
  return ec.isConnected();
}

class TestEWrapperEventCollector : public ib::internal::EWrapperEventCollector
{
 public:
  TestEWrapperEventCollector() : context_(1), socket_(context_, ZMQ_PUB)
  {
    std::string endpoint = "tcp://127.0.0.1:6666";
    LOG(INFO) << "Creating publish socket @ " << endpoint << std::endl;
    socket_.bind(endpoint.c_str());
  }

  zmq::socket_t* getOutboundSocket(int channel = 0)
  {
    return &socket_;
  }

 private:
  zmq::context_t context_;
  zmq::socket_t socket_;
};


TEST(V964_MessageTest, EClientSocketTest)
{
  boost::asio::io_service ioService;

  ApplicationBase app;

  TestEWrapperEventCollector eventCollector;
  EWrapper* ew = EWrapperFactory::getInstance(app, eventCollector);
  TestHarness* th = dynamic_cast<TestHarness*>(ew);

  AsioEClientSocket ec(ioService, *ew);

  LOG(INFO) << "Started " << ioService.run() << std::endl;
  EXPECT_TRUE(ec.eConnect("127.0.0.1", 4001, 0));

  bool connected = waitForConnection(ec, 5);
  EXPECT_TRUE(connected);

  MarketDataRequest request;

  const string tickerId = "96099040";

  // Using set(X) as the type-safe way (instead of setField())
  request.set(FIX::Symbol("GOOG"));
  request.set(FIX::SecurityType(FIX::SecurityType_OPTION));
  request.set(FIX::PutOrCall(FIX::PutOrCall_CALL));
  request.set(FIX::SecurityExchange(IBAPI::SecurityExchange_SMART));
  request.set(FIX::ContractMultiplier(100));
  request.set(FIX::StrikePrice(590.));

  // For testing, since the contract is constructed (vs. retrieved from
  // IB API calls), the conId field should not be set -- arbitrary values
  // will not match the actual conId that IB uses.
  //request.set(FIX::MDEntryRefID(tickerId));

  Date today = Date::todaysDate();
  Date nextFriday = Date::nextWeekday(today, QuantLib::Friday);

  IBAPI::V964::FormatExpiry(request, nextFriday);

  std::string dateString = FormatOptionExpiry(
      nextFriday.year(), nextFriday.month(), nextFriday.dayOfMonth());

  LOG(INFO) << "Next Friday = " << nextFriday << ", " << dateString;

  Contract contract;
  request.marshall(contract);

  LOG(INFO) << "Created contract.";

  long tid = atol(tickerId.c_str());

  LOG(INFO) << "Request market data: " << tid;

  ec.reqMktData(tid, contract, "", false);

  LOG(INFO) << "Waiting for data.";
  th->waitForFirstOccurrence(TICK_PRICE, MAX_WAIT);
  th->waitForFirstOccurrence(TICK_SIZE, MAX_WAIT);

  // Disconnect
  ec.eDisconnect();

  EXPECT_FALSE(ec.isConnected());

  EXPECT_GE(th->getCount(TICK_PRICE), 1);
  EXPECT_GE(th->getCount(TICK_SIZE), 1);
  EXPECT_TRUE(th->hasSeenTickerId(tid));
}


TEST(V964_MessageTest, EClientSocketOptionChainMktDataRequestTest)
{
  boost::asio::io_service ioService;

  ApplicationBase app;

  TestEWrapperEventCollector eventCollector;
  EWrapper* ew = EWrapperFactory::getInstance(app, eventCollector);
  TestHarness* th = dynamic_cast<TestHarness*>(ew);

  boost::shared_ptr<ib::internal::IBClient> client(
      new AsioEClientSocket(ioService, *ew));

  LOG(INFO) << "Started " << ioService.run() << std::endl;
  EXPECT_TRUE(client->eConnect("127.0.0.1", 4001, 100));

  bool connected = waitForConnection(*client, 10);
  EXPECT_TRUE(connected);

  LOG(INFO) << "Option chain request";

  OptionChainRequest request;

  // Using set(X) as the type-safe way (instead of setField())
  request.set(FIX::Symbol("AAPL"));
  request.set(FIX::SecurityExchange(IBAPI::SecurityExchange_SMART));

  // Filter for PUT only
  request.set(FIX::PutOrCall(FIX::PutOrCall_PUT));

  Date today = Date::todaysDate();
  Date nextFriday = Date::nextWeekday(today, QuantLib::Friday);

  // Filter by expiry
  IBAPI::V964::FormatExpiry(request, nextFriday);
  std::string dateString = FormatOptionExpiry(
      nextFriday.year(), nextFriday.month(), nextFriday.dayOfMonth());
  LOG(INFO) << "Next Friday = " << nextFriday << ", " << dateString;

  Contract contract;
  request.marshall(contract);

  LOG(INFO) << "Created contract.";

  EXPECT_EQ("OPT", contract.secType);
  EXPECT_EQ("AAPL", contract.symbol);
  EXPECT_EQ("SMART", contract.exchange);
  EXPECT_EQ("USD", contract.currency);
  EXPECT_EQ("P", contract.right);

  long requestId = 10000;

  std::vector<Contract> optionChain;
  th->setOptionChain(&optionChain);

  LOG(INFO) << "Request contract details: " << requestId;

  client->reqContractDetails(requestId, contract);

  LOG(INFO) << "Waiting for data.";
  th->waitForFirstOccurrence(CONTRACT_DETAILS_END, MAX_WAIT);

  EXPECT_EQ(th->getCount(CONTRACT_DETAILS_END), 1);
  EXPECT_TRUE(th->hasSeenTickerId(requestId));

  // Check the option chain
  EXPECT_GT(optionChain.size(), 0);

  LOG(INFO) << "Got " << optionChain.size() << " contracts.";

  // Get one of the contracts
  Contract c = optionChain[optionChain.size() / 2];

  TickerMap tm;
  long id1 = tm.registerContract(c);
  EXPECT_EQ(c.conId, id1);

  std::string s1;
  bool ok = tm.getSubscriptionKeyFromId(id1, &s1);
  EXPECT_TRUE(ok);

  // We already filtered by expiry...
  MarketDataRequest mdr;

  std::ostringstream tickerId;
  tickerId << c.conId;

  float multiplier = -1;
  std::istringstream iss(c.multiplier);
  iss >> multiplier;

  // Using set(X) as the type-safe way (instead of setField())
  mdr.set(FIX::Symbol(c.symbol));
  mdr.set(FIX::SecurityType(FIX::SecurityType_OPTION));
  mdr.set(FIX::PutOrCall(FIX::PutOrCall_PUT));
  mdr.set(FIX::SecurityExchange(c.exchange));
  mdr.set(FIX::ContractMultiplier(multiplier));
  mdr.set(FIX::StrikePrice(c.strike));
  mdr.set(FIX::MDEntryRefID(tickerId.str()));
  mdr.set(FIX::DerivativeSecurityID(c.localSymbol));
  mdr.set(FIX::Currency(c.currency));
  IBAPI::V964::FormatExpiry(mdr, nextFriday);

  print(mdr);

  LOG(INFO) << "MarketDataRequest = " << mdr;

  TickerMap tm2;
  Contract cc;
  mdr.marshall(cc);
  long id2 = tm2.registerContract(cc);
  EXPECT_EQ(cc.conId, id2);

  std::string s2;
  bool ok2 = tm2.getSubscriptionKeyFromId(id2, &s2);
  EXPECT_TRUE(ok2);

  EXPECT_EQ(id1, id2);
  EXPECT_EQ(s1, s2);
  EXPECT_EQ(cc.conId, c.conId);
  EXPECT_EQ(cc.conId, id2);

  LOG(INFO) << "subscription symbol1 " << s1;
  LOG(INFO) << "subscription symbol2 " << s2;

  // reset the counters
  th->resetCounters();

  // Now make request.
  bool called = mdr.callApi(client);
  EXPECT_TRUE(called);

  th->waitForFirstOccurrence(TICK_PRICE, MAX_WAIT);
  th->waitForFirstOccurrence(TICK_SIZE, MAX_WAIT);

  EXPECT_GE(th->getCount(TICK_PRICE), 1);
  EXPECT_GE(th->getCount(TICK_SIZE), 1);
  EXPECT_TRUE(th->hasSeenTickerId(c.conId));

  // Disconnect
  client->eDisconnect();
}

