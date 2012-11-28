
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

#include "common/moving_window.hpp"
#include "common/ohlc.hpp"
#include "common/ohlc_callbacks.hpp"
#include "common/time_utils.hpp"

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
using namespace atp::time_series;
using namespace atp::time_series::callback;


using atp::platform::message_processor;
using atp::platform::types::timestamp_t;
using proto::ib::MarketData;

using boost::posix_time::time_duration;


static const string DATA_DIR = "test/sample_data/";
static const string LOG_FILE = "firehose.li126-61.jenkins.log.INFO.20121004.gz";
static const string PUB_ENDPOINT = "ipc://_logreader.ipc";


using namespace atp::time_series::sampler;

typedef moving_window< double, latest<double> > mw_latest_double;


bool stop_function(const string& topic, const string& message,
                   const string& label)
{
  LOG(INFO) << "Stopping with " << label;
  return false;
}

template <typename V>
void print(const timestamp_t& ts, const V& v,
           const string& event, int* count)
{
  ptime t = atp::time::as_ptime(ts);
  LOG(INFO) << "Got " << event << " " << " = ["
            << atp::time::to_est(t) << ", " << v
            << ", ts=" << ts
            << "]";
  (*count)++;
}


namespace atp {
namespace time_series {
namespace callback {

using atp::time_series::data_series;


// partial specialization of the template
// this is required to be here because the linker can't find
// the specialization in a lib.
template <typename V>
struct logger_post_process : public ohlc_post_process<V>
{
  typedef atp::time_series::sampler::open<V> ohlc_open;
  typedef atp::time_series::sampler::close<V> ohlc_close;
  typedef atp::time_series::sampler::min<V> ohlc_low;
  typedef atp::time_series::sampler::max<V> ohlc_high;

  inline void operator()(const size_t count,
                         const data_series<V>& open,
                         const data_series<V>& high,
                         const data_series<V>& low,
                         const data_series<V>& close)
  {
    for (int i = -count; i < 0; ++i) {
      ptime t = atp::time::as_ptime(open.get_time(-2 + i));
      LOG(INFO) << atp::time::to_est(t) << ","
                << "open=" << open[-2 + i]
                << ",high=" << high[-2 + i]
                << ",low=" << low[-2 + i]
                << ",close=" << close[-2 + i];
    }
  }
};

} // callback
} // time_series
} // atp



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

TEST(IndicatorPrototype, OhlcUsage)
{
  using namespace atp::time_series;
  using namespace atp::time_series::callback;

  typedef ohlc<double, post_process_cout<double> > ohlc_last;
  typedef ohlc<double, logger_post_process<double> > ohlc_last_logging;


  int scan_seconds = 10;
  marketdata_handler<MarketData> feed_handler1;

  ohlc_last last_trade_candle(seconds(scan_seconds), seconds(1), 0.);

  // Tedious
  atp::platform::callback::update_event<double>::func d1 =
      boost::bind(&ohlc_last::on,
                  &last_trade_candle, _1, _2);
  feed_handler1.bind("LAST", d1);

  message_processor::protobuf_handlers_map symbol_handlers1;
  symbol_handlers1.register_handler("AAPL.STK", feed_handler1);
  symbol_handlers1.register_handler(
      atp::log_reader::DATA_END,
      boost::bind(&stop_function, _1, _2, "End of Log"));
  message_processor agent1(PUB_ENDPOINT, symbol_handlers1);

  // second one....
  marketdata_handler<MarketData> aapl_handler;
  int last_trade_count = 0;
  atp::platform::callback::update_event<double>::func dd =
      boost::bind(&print<double>, _1, _2, "LAST", &last_trade_count);
  aapl_handler.bind("LAST", dd);
  message_processor::protobuf_handlers_map symbol_handlers2;
  symbol_handlers2.register_handler(
      "AAPL.STK", aapl_handler);
  symbol_handlers2.register_handler(
      atp::log_reader::DATA_END,
      boost::bind(&stop_function, _1, _2, "End of Log"));
  message_processor agent2(PUB_ENDPOINT, symbol_handlers2);

  LOG(INFO) << "Starting thread";
  LogReader reader(DATA_DIR + LOG_FILE);
  boost::thread* th = atp::log_reader::DispatchEventsInThread(
      reader,
      PUB_ENDPOINT,
      seconds(scan_seconds + 10));

  th->join();
  agent1.block();
  agent2.block();
  delete th;


  /*
  sequential_pipeline<double> sp;
  mw_latest_double last_trade(seconds(scan_seconds), seconds(1), 0.);

  indicator<double> ma5;
  indicator<double> ma20;

  sp.connect(feed_handler1, "LAST")
      >> last_trade
      >> ma5
      >> ma20;

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

