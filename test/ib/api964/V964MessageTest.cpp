#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <glog/logging.h>
#include <gtest/gtest.h>
#include <zmq.hpp>
#include <stdlib.h>

#include "utils.hpp"
#include "common.hpp"
#include "ib/api964/ApiMessages.hpp"

using FIX::FieldMap;

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
  request.set(FIX::ContractMultiplier(200));
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
  EXPECT_EQ("200", c.multiplier);
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
  EXPECT_EQ("200", c2.multiplier);
  EXPECT_EQ("AAPL 20111119C00450000", c2.localSymbol);
  EXPECT_EQ(123456, c2.conId);
  EXPECT_EQ("USD", c2.currency);
}

