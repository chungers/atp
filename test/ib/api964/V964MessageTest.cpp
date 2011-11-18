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
#include "ib/ApiMessageBase.hpp"
#include "zmq/Reactor.hpp"

#include "ib/api964/ApiMessages.hpp"

using FIX::FieldMap;

using ib::internal::ZmqMessage;
using IBAPI::V964::MarketDataRequest;


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

TEST(V964MessageTest, ApiTest)
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
  const IBAPI::Header& header = request.getHeader();

  FIX::BeginString msgMatchKey; // First field, for zmq subscription matching.
  header.get(msgMatchKey);
  EXPECT_EQ("MarketDataRequest.v964.ib", msgMatchKey.getString());

  FIX::MsgType msgType;
  header.get(msgType);
  EXPECT_EQ("MarketDataRequest", msgType.getString());

  const IBAPI::Trailer& trailer = request.getTrailer();
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
  ZmqMessage zmqMessage;
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

TEST(V964MessageTest, ZmqSendTest)
{
  // Set up the reactor
  const std::string& addr =
      "ipc://_zmq.V964MessageTest_zmqSendTest.in";
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

  using QuantLib::Date;
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
