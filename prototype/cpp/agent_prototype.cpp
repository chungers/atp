
#include <string>
#include <math.h>
#include <vector>

#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <boost/thread.hpp>


#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include <glog/logging.h>

#include "utils.hpp"
#include "zmq/ZmqUtils.hpp"
#include "common/time_utils.hpp"

#include "time_series/moving_window.hpp"


#include "platform/marketdata_handler_proto_impl.hpp"
#include "platform/message_processor.hpp"

#include "service/LogReader.hpp"
#include "service/LogReaderVisitor.hpp"
#include "service/LogReaderZmq.hpp"


DEFINE_int32(scan_minutes, 4, "minutes to scan after market opens");
DEFINE_int32(scan_seconds, 60, "seconds to scan after market opens");

static const string DATA_DIR = "test/sample_data/";
static const string LOG_FILE = "firehose.log.gz";
static const string PUB_ENDPOINT = "ipc://_logreader.ipc";


DEFINE_string(data_dir, DATA_DIR, "test data directory");
DEFINE_string(log_file, LOG_FILE, "log file name");



using std::string;

using namespace boost::posix_time;
using namespace atp::log_reader;
using namespace atp::platform::callback;
using namespace atp::platform::marketdata;
using namespace atp::common;
using namespace atp::time_series;

using atp::platform::message_processor;
using atp::platform::types::timestamp_t;
using proto::ib::MarketData;

using boost::posix_time::time_duration;


bool stop_function(const string& topic, const string& message,
                   const string& label)
{
  LOG(INFO) << "Stopping with " << label;
  return false;
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
    ptime ts = atp::common::as_ptime(t);

    if (ts - last_size_pt < seconds(1)) {
      // LOG(WARNING)
      //     << "duplicate last size = "
      //     << atp::common::to_est(last_size_pt) << ","
      //     << atp::common::to_est(ts) << ","
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
  LogReader reader(FLAGS_data_dir + FLAGS_log_file);
  boost::thread* th = atp::log_reader::DispatchEventsInThread(
      reader,
      PUB_ENDPOINT,
      minutes(FLAGS_scan_minutes) + seconds(5));

  th->join();
  agent.block();

  delete th;
}



template <typename V>
void aapl(const timestamp_t& ts, const V& v,
          const string& event, int* count)
{
  ptime t = atp::common::as_ptime(ts);
  LOG(INFO) << "Got appl " << event << " " << " = ["
            << atp::common::to_est(t) << ", " << v
            << ", ts=" << ts
            << "]";
  (*count)++;
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
  const string message("End of the Log!");

  message_processor::protobuf_handlers_map symbol_handlers;
  symbol_handlers.register_handler(
      "AAPL.STK", aapl_handler);
  symbol_handlers.register_handler(
      atp::log_reader::DATA_END,
      boost::bind(&stop_function, _1, _2, boost::cref(message)));

  message_processor agent(PUB_ENDPOINT, symbol_handlers);

  LOG(INFO) << "Starting thread";
  LogReader reader(FLAGS_data_dir + FLAGS_log_file);
  boost::thread* th = atp::log_reader::DispatchEventsInThread(
      reader,
      PUB_ENDPOINT,
      seconds(30));

  th->join();
  agent.block();
  delete th;

  EXPECT_GT(bid_count, 1); // at least... don't have exact count
  EXPECT_GT(ask_count, 1); // at least... don't have exact count
}


typedef moving_window< double, sampler::latest<double> > mw_latest_double;

template <typename V>
void callOnMethod(const timestamp_t& ts, const V& v,
                  mw_latest_double* mw,
                  size_t* count_state, const size_t& limit)
{
  size_t pushed = mw->on(ts, v);
  *count_state += pushed;
  size_t count = *count_state;
  if (count > limit) {
    // dump the data out.
    microsecond_t ts[count + 1];
    double last[count + 1];

    // Note we get the last N+1 points but discard the latest
    // sample because it's the current value and may not be the
    // final pushed/aggregated value.
    EXPECT_EQ(count + 1, mw->copy_last(ts, last, count + 1));
    for (int i = 0; i < count; ++i) {
      ptime tt = atp::common::as_ptime(ts[i]);
      LOG(INFO) << "==================================== "
                << atp::common::to_est(tt) << "," << last[i];
    }
    *count_state = 0; // reset
  }
}

TEST(AgentPrototype, MovingWindowUsage)
{
  int scan_seconds = 30;

  // first handler
  marketdata_handler<MarketData> feed_handler;
  mw_latest_double last_trade(seconds(scan_seconds), seconds(1), 0.);
  atp::platform::callback::update_event<double>::func d1 =
        boost::bind(&mw_latest_double::on, &last_trade, _1, _2);
  feed_handler.bind("LAST", d1);
  message_processor::protobuf_handlers_map symbol_handlers;
  symbol_handlers.register_handler("AAPL.STK", feed_handler);
  symbol_handlers.register_handler(
      atp::log_reader::DATA_END,
      boost::bind(&stop_function, _1, _2, "End of Log"));
  message_processor agent(PUB_ENDPOINT, symbol_handlers);


  // second one....
  marketdata_handler<MarketData> aapl_handler;
  int last_trade_count = 0;
  atp::platform::callback::update_event<double>::func dd =
      boost::bind(&aapl<double>, _1, _2, "LAST", &last_trade_count);
  aapl_handler.bind("LAST", dd);
  message_processor::protobuf_handlers_map symbol_handlers2;
  symbol_handlers2.register_handler(
      "AAPL.STK", aapl_handler);
  symbol_handlers2.register_handler(
      atp::log_reader::DATA_END,
      boost::bind(&stop_function, _1, _2, "End of Log"));
  message_processor agent2(PUB_ENDPOINT, symbol_handlers2);


  // third handler
  size_t samples = 0;
  marketdata_handler<MarketData> feed_handler3;
  mw_latest_double last_trade3(seconds(scan_seconds), seconds(1), 0.);
  atp::platform::callback::update_event<double>::func f3 =
      boost::bind(&callOnMethod<double>, _1, _2, &last_trade3, &samples, 5);
  feed_handler3.bind("LAST", f3);
  message_processor::protobuf_handlers_map symbol_handlers3;
  symbol_handlers3.register_handler("AAPL.STK", feed_handler3);
  symbol_handlers3.register_handler(
      atp::log_reader::DATA_END,
      boost::bind(&stop_function, _1, _2, "End of Log"));
  message_processor agent3(PUB_ENDPOINT, symbol_handlers3);


  LOG(INFO) << "Starting thread";
  LogReader reader(FLAGS_data_dir + FLAGS_log_file);
  boost::thread* th = atp::log_reader::DispatchEventsInThread(
      reader,
      PUB_ENDPOINT,
      seconds(scan_seconds + 10));

  th->join();
  agent.block();
  delete th;

  LOG(INFO) << "total samples = " << last_trade.size();

  EXPECT_GT(last_trade.size(), 1);
  EXPECT_GT(samples, 1);

  // dump the data out...
  microsecond_t ts[last_trade.size()];
  double last[last_trade.size()];

  EXPECT_EQ(last_trade.size(),
            last_trade.copy_last(ts, last, last_trade.size()));

  for (int i = 0; i < last_trade.size(); ++i) {
    ptime tt = atp::common::as_ptime(ts[i]);
    LOG(INFO) << atp::common::to_est(tt) << "," << last[i];
  }
}

template <typename V>
class ohlc
{
 public:

  typedef typename sampler::open<V> ohlc_open;
  typedef moving_window<V, ohlc_open > mw_open;

  typedef typename sampler::close<V> ohlc_close;
  typedef moving_window<V, ohlc_close > mw_close;

  typedef typename sampler::max<V> ohlc_high;
  typedef moving_window<V, ohlc_high > mw_high;

  typedef typename sampler::min<V> ohlc_low;
  typedef moving_window<V, ohlc_low > mw_low;


  ohlc(time_duration h, time_duration s, V initial) :
      open_(h, s, initial)
      , close_(h, s, initial)
      , high_(h, s, initial)
      , low_(h, s, initial)
  {
  }

  size_t on(microsecond_t timestamp, V value)
  {
    size_t open = open_.on(timestamp, value);
    size_t close = close_.on(timestamp, value);
    size_t high = high_.on(timestamp, value);
    size_t low = low_.on(timestamp, value);
    return open;
  }

  const mw_open& open() const
  {
    return open_;
  }

  const mw_close& close() const
  {
    return close_;
  }

  const mw_high& high() const
  {
    return high_;
  }

  const mw_low& low() const
  {
    return low_;
  }

 private:

  mw_open open_;
  mw_close close_;
  mw_high high_;
  mw_low low_;
};

template <typename V>
class indicator
{

};

template <typename V>
class indicator_chain
{
 public:

  indicator_chain<V>& operator>>(indicator<V>& ind)
  {
    return *this;
  }

};

template <typename V>
class sequential_pipeline
{
 public:

  sequential_pipeline() : indicators(NULL)
  {
  }

  void on(const timestamp_t& ts, const V& v)
  {

  }

  template <typename P>
  sequential_pipeline<V>& connect(marketdata_handler<P>& h, const string& key)
  {
    callback f = boost::bind(&sequential_pipeline<V>::on, this, _1, _2);
    h.bind(key, f);
    return *this;
  }

  template <typename S>
  indicator_chain<V>& operator>>(moving_window<V,S>& mv)
  {
    return *indicators;
  }

 private:
  typedef typename atp::platform::callback::update_event<V>::func callback;

  indicator_chain<V>* indicators;
};

TEST(AgentPrototype, SequentialPipeline)
{
  int scan_seconds = 10;
  marketdata_handler<MarketData> feed_handler1;
  sequential_pipeline<double> sp;

  mw_latest_double last_trade(seconds(scan_seconds), seconds(1), 0.);

  indicator<double> ma5;
  indicator<double> ma20;

  sp.connect(feed_handler1, "LAST")
      >> last_trade
      >> ma5
      >> ma20;


  ohlc<double> last_trade_candle(seconds(scan_seconds), seconds(1), 0.);

  last_trade_candle.on(1000, 1.);
  last_trade_candle.on(1001, 2.);
  last_trade_candle.on(1002, 3.);
  last_trade_candle.on(1003, 4.);

  double open = last_trade_candle.open()[-1];
  double close = last_trade_candle.close()[-1];
  double high = last_trade_candle.high()[-1];
  double low = last_trade_candle.low()[-1];

  EXPECT_EQ(1., open);
  EXPECT_EQ(4., close);
  EXPECT_EQ(4., high);
  EXPECT_EQ(1., low);


  // pseudo code
  /*
  ohlc<double> spx_candle(seconds(scan_seconds), seconds(1), 0.);

  indicator<double> sma5 = sma(last_trade_candle.close(), 5);
  indicator<double> sma20 = sma(last_trade_candle.close(), 20);
  indicator<double> spx_sma5 = sma(spx_candle.close(), 5);
  indicator<double> spx_sma20 = sma(spx_candle.close(), 20);
  marketdata_feed.subscribe("AAPL.STK").event("LAST")
      >> last_trade_candle
      >> sma5
      >> sma20;
  marketdata_feed.subscribe("SPX.IND").event("LAST")
      >> spx_candle
      >> spx_sma5,
      >> spx_sma20;
  */
}
