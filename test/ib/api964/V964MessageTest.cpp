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

TEST(V964MessageTest, ApiTest)
{
  LOG(INFO) << "Current TimeMicros = " << now_micros() ;

  MarketDataRequest request;

  // Using set(X) as the type-safe way (instead of setField())
  request.set(FIX::SecurityType(FIX::SecurityType_OPTION));
  request.set(FIX::PutOrCall(FIX::PutOrCall_PUT));

  FIX::Symbol aapl("AAPL");
  request.set(aapl);
  request.set(FIX::StrikePrice(425.50));  // Not a valid strike but for testing


  // This will not compile -- field is not defined.
  //request.set(IBAPI::SecurityExchange("SMART"));


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

  FIX::PutOrCall putOrCall;
  request.get(putOrCall);
  EXPECT_EQ(FIX::PutOrCall_PUT, putOrCall);

  FIX::StrikePrice strike;
  request.get(strike);
  EXPECT_EQ("425.5", strike.getString());
  EXPECT_EQ(425.5, strike);

  // Get header information
  const IBAPI::Header& header = request.getHeader();
  FIX::MsgType msgType;
  header.get(msgType);

  EXPECT_EQ(msgType.getString(), "MarketDataRequest");

  const IBAPI::Trailer& trailer = request.getTrailer();
  IBAPI::Ext_SendingTimeMicros sendingTimeMicros;
  trailer.get(sendingTimeMicros);

  LOG(INFO) << "Timestamp = " << sendingTimeMicros.getString() ;
}

