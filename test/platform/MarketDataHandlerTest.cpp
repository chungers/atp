

#include <string>

#include <boost/bind.hpp>

#include <zmq.hpp>

#include <gtest/gtest.h>
#include <glog/logging.h>

#include "utils.hpp"
#include "zmq/ZmqUtils.hpp"

#include "proto/common.pb.h"
#include "proto/ib.pb.h"
#include "platform/marketdata_handler.hpp"


using std::string;

using namespace atp::platform;
using proto::common::Value;
using proto::ib::MarketData;


namespace atp {
namespace platform {

namespace message {

template <>
inline bool parse(const string& raw, MarketData& m)
{
  return m.ParseFromString(raw);
}

template <>
inline const timestamp_t get_timestamp<MarketData>(const MarketData& m)
{
  return m.timestamp();
}

template <>
inline const string& get_event_code<MarketData, string>(const MarketData& m)
{
  return m.event();
}

template <>
bool value_updater<MarketData, string>::operator()(const timestamp_t& ts,
                                                   const string& event_code,
                                                   const MarketData& event)
{
  bool processed = false;

  switch (event.value().type()) {
    case proto::common::Value::DOUBLE: {
      double_updaters::iterator itr = double_updaters_.find(event_code);
      if (itr != double_updaters_.end()) {
        itr->second(ts, event.value().double_value());
        processed = true;
      }
      break;
    }

    case proto::common::Value::INT: {
      int_updaters::iterator itr = int_updaters_.find(event_code);
      if (itr != int_updaters_.end()) {
        itr->second(ts, event.value().int_value());
        processed = true;
      }
      break;
    }

    case proto::common::Value::STRING: {
      string_updaters::iterator itr = string_updaters_.find(event_code);
      if (itr != string_updaters_.end()) {
        itr->second(ts, event.value().string_value());
        processed = true;
      }
      break;
    }
  }
  return processed;
}

} // message
} // platform
} // atp


bool aapl(const atp::platform::timestamp_t& ts, const double& v,
          const string& event, int* count)
{
  LOG(INFO) << "Got appl " << event << " " << " = [" << ts << ", " << v << "]";
  (*count)++;
  return true;
}

TEST(MarketDataHandlerTest, UsageSyntax)
{

  marketdata_handler<MarketData> h;

  MarketData bid;
  bid.set_timestamp(now_micros());
  bid.set_symbol("AAPL.STK");
  bid.set_event("BID");
  bid.mutable_value()->set_type(proto::common::Value::DOUBLE);
  bid.mutable_value()->set_double_value(600.);
  bid.set_contract_id(12345);

  MarketData ask;
  ask.set_timestamp(now_micros());
  ask.set_symbol("AAPL.STK");
  ask.set_event("ASK");
  ask.mutable_value()->set_type(proto::common::Value::DOUBLE);
  ask.mutable_value()->set_double_value(601.);
  ask.set_contract_id(12345);

  string buff, buff2;
  EXPECT_TRUE(bid.SerializeToString(&buff));
  EXPECT_TRUE(ask.SerializeToString(&buff2));

  EXPECT_FALSE(h.process_event("AAPL.STK", buff)); // no handlers

  int bid_count = 0, ask_count = 0;

  atp::platform::callback::double_updater d1 = boost::bind(&aapl, _1, _2,
                                                          "BID", &bid_count);
  atp::platform::callback::double_updater d2 = boost::bind(&aapl, _1, _2,
                                                          "ASK", &ask_count);
  h.bind("BID", d1);
  h.bind("ASK", d2);

  EXPECT_TRUE(h.process_event("AAPL.STK", buff)); // has handler
  EXPECT_TRUE(h.process_event("AAPL.STK", buff)); // has handler

  EXPECT_EQ(2, bid_count);
  EXPECT_EQ(0, ask_count);

  EXPECT_TRUE(h.process_event("AAPL.STK", buff2)); // has handler
  EXPECT_TRUE(h.process_event("AAPL.STK", buff2)); // has handler

  EXPECT_EQ(2, bid_count);
  EXPECT_EQ(2, ask_count);

}
