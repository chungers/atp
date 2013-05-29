
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <boost/tuple/tuple.hpp>

#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include <glog/logging.h>

#include <ta-lib/ta_func.h>

#include "time_series/moving_window.hpp"
#include "time_series/moving_window_callbacks.hpp"
#include "time_series/ohlc.hpp"
#include "time_series/ohlc_callbacks.hpp"

#include "platform/control_message_handler.hpp"
#include "platform/marketdata_handler_proto_impl.hpp"
#include "platform/message_processor.hpp"

#include "service/LogReader.hpp"
#include "service/LogReaderVisitor.hpp"
#include "service/LogReaderZmq.hpp"



using std::cout;
using std::string;

using namespace boost::posix_time;
using namespace atp::log_reader;
using namespace atp::platform::callback;
using namespace atp::platform::marketdata;
using namespace atp::common;
using namespace atp::time_series;
using namespace atp::time_series::callback;
using namespace atp::time_series::sampler;

using atp::platform::message_processor;
using atp::platform::types::timestamp_t;
using atp::platform::generic_handler;

using proto::ib::MarketData;
using proto::platform::ControlMessage;

using boost::posix_time::time_duration;

static const string DATA_DIR = "test/sample_data/";
static const string LOG_FILE = "firehose.log.gz";
static const string PUB_ENDPOINT = "ipc://_logreader.ipc";


DEFINE_string(data_dir, DATA_DIR, "test data directory");
DEFINE_string(log_file, LOG_FILE, "log file name");


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
  ptime t = atp::common::as_ptime(ts);
  LOG(INFO) << "Got " << event << " " << " = ["
            << atp::common::to_est(t) << ", " << v
            << ", ts=" << ts
            << "]";
  (*count)++;
}


namespace atp {
namespace common {
namespace callback {

using atp::time_series::time_series;
using atp::common::microsecond_t;


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
                         const Id& id,
                         const time_series<microsecond_t, V>& open,
                         const time_series<microsecond_t, V>& high,
                         const time_series<microsecond_t, V>& low,
                         const time_series<microsecond_t, V>& close)
  {
    for (int i = -count; i < 0; ++i) {
      ptime t = atp::common::as_ptime(open.t[i]);
      cout << atp::common::to_est(t) << ","
           << open[i] << ","
           << high[i] << ","
           << low[i] << ","
           << close[i] << std::endl;
    }
  }
};
} // callback
} // common
} // atp


struct sma
{
  sma(int period) : period(period) {}

  double operator()(const microsecond_t& t,
                    const double* series, const size_t len)
  {
    // for (size_t i = 0; i < len; ++i) {
    //   LOG(INFO) << "series " << len << ", " << series[i];
    // }
    TA_Real out[len];
    TA_Integer outIndex, outCount;

    int ret = TA_MA(0, len-1, series, period, TA_MAType_SMA,
                    &outIndex, &outCount, &out[0]);
    // for (size_t i = 0; i < len; ++i) {
    //   LOG(INFO) << "ma " << len << ", " << out[i];
    // }

    // LOG(INFO) << "sma = " << outIndex << ", "
    //            << outCount << ", " << out[outCount - 1];
    return out[outCount - 1];
  }

  int period;
};


typedef ohlc<double> ohlc_t;

// MACD, MACD-signal, MACD-histogram
typedef boost::tuple<double, double, double> macd_value_t;
struct MACD : public moving_window_post_process<microsecond_t, macd_value_t>
{
  typedef
  moving_window<macd_value_t, atp::time_series::sampler::close<macd_value_t> >
  macd_series;

  MACD(int fastPeriod, int slowPeriod, int signalPeriod,
       macd_series& series) :
      fastPeriod(fastPeriod),
      slowPeriod(slowPeriod),
      signalPeriod(signalPeriod),
      series(series) {}

  virtual ~MACD() {}

  double operator()(const microsecond_t& t,
                    const double* price,
                    const size_t len)
  {
    double outMACD[len];
    double outMACDSignal[len];
    double outMACDHist[len];
    int outBegIdx = 0;
    int outNBElement = 0;
    int ret = TA_MACD(0, len-1, price, fastPeriod, slowPeriod, signalPeriod,
                      &outBegIdx, &outNBElement,
                      &outMACD[0], &outMACDSignal[0], &outMACDHist[0]);

    return series(t, macd_value_t(outMACD[outNBElement-1],
                                  outMACDSignal[outNBElement-1],
                                  outMACDHist[outNBElement-1]));
  }

  virtual void operator()(const size_t count,
                          const Id& id,
                          const time_series<microsecond_t, macd_value_t>& w)
  {
    for (int i = -count; i < 0; ++i) {
      ptime t = atp::common::as_ptime(w.get_time(i));
      cout << atp::common::to_est(t) << ","
           << id << ","
           << boost::get<0>(w[i]) << ','
           << boost::get<1>(w[i]) << ','
           << boost::get<2>(w[i])
           << endl;
    }
  }

  int fastPeriod, slowPeriod, signalPeriod;
  macd_series& series;
};



struct trader : public moving_window_post_process<microsecond_t, microsecond_t>
{
  typedef atp::platform::callback::update_event<double>::func updater;
  typedef moving_window<
    microsecond_t, atp::time_series::sampler::close<microsecond_t> > timer_t;

  trader(const Id& _id, int bars, int seconds_per_bar) :
      id(_id),
      ohlc(seconds(bars * seconds_per_bar),
           seconds(seconds_per_bar), 0.),
      ohlc2(seconds(bars * seconds_per_bar),
            seconds(seconds_per_bar), 0.),
      mid(seconds(bars * seconds_per_bar),
          seconds(seconds_per_bar), 0.),
      bid(seconds(bars * seconds_per_bar),
          seconds(seconds_per_bar), 0.),
      ask(seconds(bars * seconds_per_bar),
          seconds(seconds_per_bar), 0.),
      mid2(seconds(bars * seconds_per_bar),
           seconds(seconds_per_bar), 0.),
      bid2(seconds(bars * seconds_per_bar),
           seconds(seconds_per_bar), 0.),
      ask2(seconds(bars * seconds_per_bar),
           seconds(seconds_per_bar), 0.),
      ten_sec_timer(seconds(bars),
                    seconds(10), 0),
      sma5(5), sma20(20),
      macd(seconds(bars * seconds_per_bar),
           seconds(seconds_per_bar), 0.),
      macd_pp(5, 20, 20, macd)
  {
    /////////////////////////////////////
    id.set_source("AAPL.STK");
    id.add_label("last$ohlc");
    ohlc.set(id);
    ohlc.set(ohlc_pp);
    ohlc.mutable_close().set(mv_pp);
    ta_sma5 = &ohlc.mutable_close().apply3("sma5", sma5, mv_pp);
    ta_sma20 = &ohlc.mutable_close().apply3("sma20", sma20, mv_pp);

    Id midId = id;
    midId.add_label("mid");
    mid.set(midId);
    mid.set(mv_pp);

    Id bidId = id;
    bidId.add_label("bid");
    bid.set(bidId);
    bid.set(mv_pp);

    Id askId = id;
    askId.add_label("ask");
    ask.set(askId);
    ask.set(mv_pp);

    /////////////////////////////////////
    Id id2 = id;
    id2.set_source("GOOG.STK");
    id2.add_label("ohlc");
    ohlc2.set(id2);
    ohlc2.set(ohlc_pp);
    ohlc2.mutable_close().set(mv_pp);

    Id midId2 = id;
    midId2.set_source("GOOG.STK");
    midId2.add_label("mid");
    mid2.set(midId2);
    mid2.set(mv_pp);

    Id bidId2 = id;
    bidId2.set_source("GOOG.STK");
    bidId2.add_label("bid");
    bid2.set(bidId2);
    bid2.set(mv_pp);

    Id askId2 = id;
    askId2.set_source("GOOG.STK");
    askId2.add_label("ask");
    ask2.set(askId2);
    ask2.set(mv_pp);

    Id timerId2 = id;
    timerId2.add_label("trade-eval-10-sec");
    ten_sec_timer.set(timerId2);
    ten_sec_timer.set(*this);

    Id macdId = id;
    macdId.add_label("last$ohlc$close$macd");
    macd.set(macdId);
    macd.set(macd_pp);

    // connect the close to macd
    ohlc.mutable_close().apply3("macd", macd_pp);
  }

  virtual void operator()(const size_t count,
                          const Id& id,
                          const time_series<
                          microsecond_t,
                          microsecond_t>& window)
  {
    if (count > 0) {

      string indicator = ((*ta_sma5)[0] > (*ta_sma20)[0]) ? ",1" : ",0";

      ptime t = atp::common::as_ptime(window.get_time(0));
      ptime t2 = atp::common::as_ptime(window.get_time(-1));
      cout << atp::common::to_est(t2) << ","
           << atp::common::to_est(t) << ","
           << id << "," << (*ta_sma5)[0] << "," << (*ta_sma20)[0]
           << indicator
           << std::endl;
    }
  }


  void on_last(const microsecond_t& t, const double& v)
  {
    ptime tt = atp::common::as_ptime(t);
    cout << atp::common::to_est(tt) << ","
         << id.name() << ","
         << id.variant() << ","
         << id.source() << ","
         << "last" << ","
         << v << std::endl;
    ohlc(t, v);
    ten_sec_timer(t, t);
  }


  void on_bid(const microsecond_t& t, const double& v)
  {
    // ptime tt = atp::common::as_ptime(t);
    // cout << atp::common::to_est(tt) << ","
    //      << id.name() << ","
    //      << id.variant() << ","
    //      << id.source() << ","
    //      << "bid-inst" << ","
    //      << v << std::endl;
    bid(t, v);
    mid(t, (ask[0] + bid[0]) / 2.);
    ten_sec_timer(t, t);
  }

  void on_ask(const microsecond_t& t, const double& v)
  {
    // ptime tt = atp::common::as_ptime(t);
    // cout << atp::common::to_est(tt) << ","
    //      << id.name() << ","
    //      << id.variant() << ","
    //      << id.source() << ","
    //      << "ask-inst" << ","
    //      << v << std::endl;
    ask(t, v);
    mid(t, (ask[0] + bid[0]) / 2.);
    ten_sec_timer(t, t);
  }

  void on_last2(const microsecond_t& t, const double& v)
  {
    ptime tt = atp::common::as_ptime(t);
    cout << atp::common::to_est(tt) << ","
         << id.name() << ","
         << id.variant() << ","
         << "GOOG.STK" << ","
         << "last" << ","
         << v << std::endl;
    ohlc2(t, v);
    ten_sec_timer(t, t);
  }


  void on_bid2(const microsecond_t& t, const double& v)
  {
    bid2(t, v);
    mid2(t, (ask2[0] + bid2[0]) / 2.);
    ten_sec_timer(t, t);
  }

  void on_ask2(const microsecond_t& t, const double& v)
  {
    ask2(t, v);
    mid2(t, (ask2[0] + bid2[0]) / 2.);
    ten_sec_timer(t, t);
  }

  int on_control(const atp::platform::types::timestamp_t& t,
                 const ControlMessage& control)
  {
    LOG(INFO) << "Got " << t << ", " << control.target();
    return 0;
  }

  void setup(message_processor::protobuf_handlers_map& map)
  {
    feed_handler1.bind(
        "LAST",
        static_cast<updater>(boost::bind(&trader::on_last, this, _1, _2)));
    feed_handler1.bind(
        "BID",
        static_cast<updater>(boost::bind(&trader::on_bid, this, _1, _2)));
    feed_handler1.bind(
        "ASK",
        static_cast<updater>(boost::bind(&trader::on_ask, this, _1, _2)));

    feed_handler2.bind(
        "LAST",
        static_cast<updater>(boost::bind(&trader::on_last2,
                                         this, _1, _2)));
    feed_handler2.bind(
        "BID",
        static_cast<updater>(boost::bind(&trader::on_bid2,
                                         this, _1, _2)));
    feed_handler2.bind(
        "ASK",
        static_cast<updater>(boost::bind(&trader::on_ask2,
                                         this, _1, _2)));
    map.register_handler("AAPL.STK", feed_handler1);

    // pretend GOOG is the index or the other half of the pair
    map.register_handler("GOOG.STK", feed_handler2);

    // control message
    control_handler.bind(control_handler.message_key(),
                         boost::bind(&trader::on_control, this, _1, _2));
    map.register_handler(control_handler.message_key(),
                         control_handler);
  }

  Id id;
  post_process_cout<double> ohlc_pp;
  moving_window_post_process_cout<microsecond_t, double> mv_pp;
  ohlc_t ohlc, ohlc2;
  moving_window<double, atp::time_series::sampler::close<double> > mid, bid, ask;
  moving_window<double, atp::time_series::sampler::close<double> > mid2, bid2, ask2;

  timer_t ten_sec_timer; // timer is just for tracking time and calling trades

  marketdata_handler<MarketData> feed_handler1, feed_handler2;
  generic_handler<ControlMessage> control_handler;

  sma sma5;
  time_series<microsecond_t, double>* ta_sma5;
  sma sma20;
  time_series<microsecond_t, double>* ta_sma20;

  moving_window<macd_value_t, atp::time_series::sampler::close<macd_value_t> > macd;
  MACD macd_pp;
};

TEST(OhlcPrototype, OhlcUsage)
{
  using namespace atp::time_series;
  using namespace atp::time_series::callback;

  message_processor::protobuf_handlers_map symbol_handlers1;
  symbol_handlers1.register_handler(
      atp::log_reader::DATA_END,
      boost::bind(&stop_function, _1, _2, "End of Log"));

  ////////////////////////////////////////
  // A typical trader class that contains ohlc and other indicators

  int bars = 60; // seconds
  int bar_interval = 10;
  int scan_seconds = bars * bar_interval * 5;

  Id id;
  id.set_name("trader1");
  id.set_variant("test1");
  trader trader(id, bars, bar_interval);

  trader.setup(symbol_handlers1);


  message_processor subscriber(PUB_ENDPOINT, symbol_handlers1);

  LOG(INFO) << "Starting thread";
  LogReader reader(FLAGS_data_dir + FLAGS_log_file);
  boost::thread* th = atp::log_reader::DispatchEventsInThread(
      reader,
      PUB_ENDPOINT,
      seconds(scan_seconds));

  th->join();
  subscriber.block();
  delete th;
}
