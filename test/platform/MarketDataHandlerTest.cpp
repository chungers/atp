

#include <string>

#include <boost/bind.hpp>

#include <zmq.hpp>

#include <gtest/gtest.h>
#include <glog/logging.h>

#include "utils.hpp"
#include "zmq/ZmqUtils.hpp"

#include "proto/common.pb.h"
#include "proto/ib.pb.h"
#include "platform/marketdata_handler_proto_impl.hpp"


using std::string;

using namespace atp::platform::callback;
using namespace atp::platform::marketdata;
using atp::platform::types::timestamp_t;
using proto::ib::MarketData;
using proto::common::Value;



void aapl(const timestamp_t& ts, const double& v,
          const string& event, int* count)
{
  LOG(INFO) << "Got appl " << event << " " << " = [" << ts << ", " << v << "]";
  (*count)++;
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

  EXPECT_EQ(atp::platform::marketdata::error_code::NO_UPDATER_CONTINUE,
            h.process_event("AAPL.STK", buff)); // no handlers

  int bid_count = 0, ask_count = 0;

  atp::platform::callback::update_event<double>::func d1 =
      boost::bind(&aapl, _1, _2, "BID", &bid_count);
  atp::platform::callback::update_event<double>::func d2 =
      boost::bind(&aapl, _1, _2, "ASK", &ask_count);
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
