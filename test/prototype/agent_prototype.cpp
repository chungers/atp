
#include <string>
#include <math.h>

#include <boost/bind.hpp>
#include <boost/thread.hpp>


#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include <glog/logging.h>

#include "utils.hpp"
#include "zmq/ZmqUtils.hpp"

#include "common/moving_window.hpp"

#include "historian/time_utils.hpp"

#include "platform/marketdata_handler_proto_impl.hpp"
#include "platform/message_processor.hpp"

#include "service/LogReader.hpp"
#include "service/LogReaderVisitor.hpp"
#include "service/LogReaderZmq.hpp"


DEFINE_int32(scan_minutes, 4, "minutes to scan after market opens");
DEFINE_int32(scan_seconds, 60, "seconds to scan after market opens");


using std::string;

using namespace boost::posix_time;
using namespace atp::log_reader;
using namespace atp::platform::callback;
using namespace atp::platform::marketdata;
using atp::platform::message_processor;
using atp::platform::types::timestamp_t;
using proto::ib::MarketData;


static const string DATA_DIR = "test/sample_data/";
static const string LOG_FILE = "firehose.li126-61.jenkins.log.INFO.20121004.gz";
static const string PUB_ENDPOINT = "ipc://_logreader.ipc";


template <typename V>
void aapl(const timestamp_t& ts, const V& v,
          const string& event, int* count)
{
  ptime t = historian::as_ptime(ts);
  LOG(INFO) << "Got appl " << event << " " << " = ["
            << historian::to_est(t) << ", " << v << "]";
  (*count)++;
}


bool stop_function(const string& topic, const string& message,
                   const string& label)
{
  LOG(INFO) << "Stopping with " << label;
  return false;
}


TEST(AgentPrototype, LoadDataFromLogfile)
{
  // create the message_processor and the marketdata_handlers
  marketdata_handler<MarketData> aapl_handler;

  // states
  int bid_count = 0, ask_count = 0, last_trade_count, last_size_count = 0;

  atp::platform::callback::update_event<double>::func d1 =
       boost::bind(&aapl<double>, _1, _2, "BID", &bid_count);

  atp::platform::callback::update_event<double>::func d2 =
      boost::bind(&aapl<double>, _1, _2, "ASK", &ask_count);

  atp::platform::callback::update_event<double>::func d3 =
      boost::bind(&aapl<double>, _1, _2, "LAST", &last_trade_count);

  atp::platform::callback::update_event<int>::func d4 =
      boost::bind(&aapl<int>, _1, _2, "LAST_SIZE", &last_size_count);


  aapl_handler.bind("BID", d1);
  aapl_handler.bind("ASK", d2);
  aapl_handler.bind("LAST", d3);
  aapl_handler.bind("LAST_SIZE", d4);

  // now message_processor
  message_processor::protobuf_handlers_map symbol_handlers;
  symbol_handlers.register_handler(
      "AAPL.STK", aapl_handler);
  symbol_handlers.register_handler(
      atp::log_reader::DATA_END,
      boost::bind(&stop_function, _1, _2, "End of Log"));

  message_processor agent(PUB_ENDPOINT, symbol_handlers);

  LOG(INFO) << "Starting thread";
  LogReader reader(DATA_DIR + LOG_FILE);
  boost::thread* th = atp::log_reader::DispatchEventsInThread(
      reader,
      PUB_ENDPOINT,
      seconds(5));

  th->join();
  agent.block();
  delete th;

  EXPECT_GT(bid_count, 1); // at least... don't have exact count
  EXPECT_GT(ask_count, 1); // at least... don't have exact count
}

/// http://epchan.blogspot.com/2012/10/order-flow-as-predictor-of-return.html
class order_flow_tracker
{
 public:
  order_flow_tracker(const string& symbol) :
      symbol(symbol), bid(0.), ask(0.), last(0.),
      bid_size(0), ask_size(0), last_size(0),
      score(0),
      incremental_order_flow(0)
  {
    using namespace atp::platform::callback;

    {
      update_event<double>::func f =
          boost::bind(&order_flow_tracker::update_bid, this, _1, _2);
      handler.bind("BID", f);
    }

    {
      update_event<double>::func f =
          boost::bind(&order_flow_tracker::update_ask, this, _1, _2);
      handler.bind("ASK", f);
    }

    {
      update_event<double>::func f =
          boost::bind(&order_flow_tracker::update_last, this, _1, _2);
      handler.bind("LAST", f);
    }

    {
      update_event<int>::func f =
          boost::bind(&order_flow_tracker::update_last_size, this, _1, _2);
      handler.bind("LAST_SIZE", f);
    }

    {
      update_event<int>::func f =
          boost::bind(&order_flow_tracker::update_bid_size, this, _1, _2);
      handler.bind("BID_SIZE", f);
    }

    {
      update_event<int>::func f =
          boost::bind(&order_flow_tracker::update_ask_size, this, _1, _2);
      handler.bind("ASK_SIZE", f);
    }

  }

  const string& get_symbol() const
  {
    return symbol;
  }

  marketdata_handler<MarketData>& get_handler()
  {
    return handler;
  }

  void update_bid(const timestamp_t& t, const double& v)
  {
    bid = v;
  }

  void update_ask(const timestamp_t& t, const double& v)
  {
    ask = v;
  }

  void update_bid_size(const timestamp_t& t, const int& v)
  {
    bid_size = v;
  }

  void update_ask_size(const timestamp_t& t, const int& v)
  {
    ask_size = v;
  }

  void update_last(const timestamp_t& t, const double& v)
  {
    last = v;
  }

  void update_last_size(const timestamp_t& t, const int& v)
  {
    ptime ts = historian::as_ptime(t);

    if (ts - last_size_pt < seconds(1)) {
      // LOG(WARNING)
      //     << "duplicate last size = "
      //     << historian::to_est(last_size_pt) << ","
      //     << historian::to_est(ts) << ","
      //     << v;
      return; // ignore -- duplicate data
    }

    last_size_pt = ts;
    last_size = v;

    double from_ask = abs(ask - last);
    double from_bid = abs(last - bid);
    if (last == ask) {
      incremental_order_flow = last_size;
      score += incremental_order_flow;
    } else if (last == bid) {
      incremental_order_flow = -last_size;
      score += incremental_order_flow;
    } else {
      incremental_order_flow = 0;
    }
    LOG(INFO) << "score = " << score
              << ",inc_flow = " << incremental_order_flow
              << ",bid = " << bid << "@" << bid_size
              << ",ask = " << ask << "@" << ask_size
              << ",last = " << last;
  }

 private:
  string symbol;
  double bid, ask, last;
  int bid_size, ask_size, last_size;
  int score;
  int incremental_order_flow;
  ptime last_size_pt;

  marketdata_handler<MarketData> handler;
};

TEST(AgentPrototype, OrderFlowTracking)
{
  // algo
  order_flow_tracker tracker("AAPL.STK");

  // now message_processor
  message_processor::protobuf_handlers_map symbol_handlers;
  symbol_handlers.register_handler(tracker.get_symbol(),
                                   tracker.get_handler());
  symbol_handlers.register_handler(
      atp::log_reader::DATA_END,
      boost::bind(&stop_function, _1, _2, "End of Log"));

  message_processor agent(PUB_ENDPOINT, symbol_handlers);

  LOG(INFO) << "Starting thread";
  LogReader reader(DATA_DIR + LOG_FILE);
  boost::thread* th = atp::log_reader::DispatchEventsInThread(
      reader,
      PUB_ENDPOINT,
      minutes(FLAGS_scan_minutes) + seconds(5));

  th->join();
  agent.block();

  delete th;
}



