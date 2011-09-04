
#include <glog/logging.h>
#include <gtest/gtest.h>

#include "utils.hpp"
#include "ib/IBAPIFields.hpp"
#include "ib/IBAPIValues.hpp"
#include "ib/Message.hpp"

using FIX::FieldMap;

class MarketDataRequest : public IBAPI::Message
{
 public:
  MarketDataRequest() : Message("MarketDataRequest")
  {
  }

  FIELD_SET(*this, IBAPI::Symbol);
  FIELD_SET(*this, IBAPI::SecurityID);
  FIELD_SET(*this, IBAPI::SecurityType);
  FIELD_SET(*this, IBAPI::PutOrCall);
  FIELD_SET(*this, IBAPI::StrikePrice);
  FIELD_SET(*this, IBAPI::MaturityDay);
  FIELD_SET(*this, IBAPI::MaturityMonthYear);
};

TEST(MessageTest, ApiTest)
{
  LOG(INFO) << "Current TimeMicros = " << now_micros() << std::endl;

  MarketDataRequest request;

  request.setField(IBAPI::SecurityType("OPT"));
  request.setField(IBAPI::PutOrCall(IBAPI::PutOrCall_PUT));
  request.setField(IBAPI::Symbol("AAPL"));

  for (FIX::FieldMap::iterator itr = request.begin();
       itr != request.end();
       ++itr) {
    LOG(INFO) << "[" << itr->second.getField() << ":"
              << "\"" << itr->second.getString() << "\"]" << std::endl;
  }

  IBAPI::PutOrCall putOrCall;
  request.getField(putOrCall);
  EXPECT_EQ(IBAPI::PutOrCall_PUT, putOrCall);

  // Get header information
  const IBAPI::Header& header = request.getHeader();
  IBAPI::MsgType msgType;
  header.getField(msgType);

  EXPECT_EQ(msgType.getString(), "MarketDataRequest");

  const IBAPI::Trailer& trailer = request.getTrailer();
  IBAPI::Ext_SendingTimeMicros sendingTimeMicros;
  trailer.getField(sendingTimeMicros);

  LOG(INFO) << "Timestamp = " << sendingTimeMicros.getString() << std::endl;
}
