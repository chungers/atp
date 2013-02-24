
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <boost/thread.hpp>

#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include <glog/logging.h>

#include "common/moving_window.hpp"
#include "common/moving_window_callbacks.hpp"
#include "common/ohlc.hpp"
#include "common/ohlc_callbacks.hpp"

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
using namespace atp::common::callback;
using namespace atp::common::sampler;

using atp::platform::message_processor;
using atp::platform::types::timestamp_t;
using proto::ib::MarketData;

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
  ptime t = atp::time::as_ptime(ts);
  LOG(INFO) << "Got " << event << " " << " = ["
            << atp::time::to_est(t) << ", " << v
            << ", ts=" << ts
            << "]";
  (*count)++;
}


namespace atp {
namespace common {
namespace callback {

using atp::common::time_series;
using atp::common::microsecond_t;


// partial specialization of the template
// this is required to be here because the linker can't find
// the specialization in a lib.
template <typename V>
struct logger_post_process : public ohlc_post_process<V>
{
  typedef atp::common::sampler::open<V> ohlc_open;
  typedef atp::common::sampler::close<V> ohlc_close;
  typedef atp::common::sampler::min<V> ohlc_low;
  typedef atp::common::sampler::max<V> ohlc_high;

  inline void operator()(const size_t count,
                         const Id& id,
                         const time_series<microsecond_t, V>& open,
                         const time_series<microsecond_t, V>& high,
                         const time_series<microsecond_t, V>& low,
                         const time_series<microsecond_t, V>& close)
  {
    for (int i = -count; i < 0; ++i) {
      ptime t = atp::time::as_ptime(open.t[i]);
      cout << atp::time::to_est(t) << ","
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


typedef ohlc<double> ohlc_t;

struct trader : public moving_window_post_process<microsecond_t, double>
{
  typedef atp::platform::callback::update_event<double>::func updater;

  trader(const Id& id, int bars, int seconds_per_bar) :
      id(id),
      ohlc(seconds(bars * seconds_per_bar),
           seconds(seconds_per_bar), 0.),
      mid(seconds(bars * seconds_per_bar),
          seconds(seconds_per_bar), 0.),
      bid(seconds(bars * seconds_per_bar),
          seconds(seconds_per_bar), 0.),
      ask(seconds(bars * seconds_per_bar),
          seconds(seconds_per_bar), 0.),
      timer(seconds(bars),
            seconds(1), 0.)
  {
    ohlc.set(id);
    ohlc.set(ohlc_pp);
       ohlc.mutable_close().set(mv_pp);

    Id midId = id;
    midId.set_label("mid");
    mid.set(midId);
    mid.set(mv_pp);

    Id bidId = id;
    bidId.set_label("bid");
    bid.set(bidId);
    bid.set(mv_pp);

    Id askId = id;
    askId.set_label("ask");
    ask.set(askId);
    ask.set(mv_pp);

    Id timerId = id;
    timerId.set_label("trade-eval");
    timer.set(timerId);
    timer.set(*this);
  }

  virtual void operator()(const size_t count,
                          const Id& id,
                          const time_series<microsecond_t, double>& window)
  {
    if (count > 0) {
      ptime t = atp::time::as_ptime(window.get_time(0));
      ptime t2 = atp::time::as_ptime(window.get_time(-1));
      cout << atp::time::to_est(t2) << ","
           << atp::time::to_est(t) << ","
           << id << " ==> evaluate trade ==> "
           << "current bid = " << bid[0] << ", ask = " << ask[0]
           << ", mid = " << mid[0] << ", last = " << ohlc.close()[0]
           << std::endl;
    }
  }

  /// receives update of LAST
  void on_last(const microsecond_t& t, const double& v)
  {
    ptime tt = atp::time::as_ptime(t);
    cout << atp::time::to_est(tt) << ","
         << id.name() << ","
         << id.variant() << ","
         << id.signal() << ","
         << "last" << ","
         << v << std::endl;
    ohlc(t, v);
    timer(t, v);
  }

  /// receives update of LAST
  void on_bid(const microsecond_t& t, const double& v)
  {
    // ptime tt = atp::time::as_ptime(t);
    // cout << atp::time::to_est(tt) << ","
    //      << id.name() << ","
    //      << id.variant() << ","
    //      << id.signal() << ","
    //      << "bid-inst" << ","
    //      << v << std::endl;
    bid(t, v);
    mid(t, (ask[0] + bid[0]) / 2.);
    timer(t, v);
  }

  void on_ask(const microsecond_t& t, const double& v)
  {
    // ptime tt = atp::time::as_ptime(t);
    // cout << atp::time::to_est(tt) << ","
    //      << id.name() << ","
    //      << id.variant() << ","
    //      << id.signal() << ","
    //      << "ask-inst" << ","
    //      << v << std::endl;
    ask(t, v);
    mid(t, (ask[0] + bid[0]) / 2.);
    timer(t, v);
  }

  void bind(marketdata_handler<MarketData>& feed_handler)
  {
    feed_handler.bind(
        "LAST",
        static_cast<updater>(boost::bind(&trader::on_last, this, _1, _2)));
    feed_handler.bind(
        "BID",
        static_cast<updater>(boost::bind(&trader::on_bid, this, _1, _2)));
    feed_handler.bind(
        "ASK",
        static_cast<updater>(boost::bind(&trader::on_ask, this, _1, _2)));
  }

  Id id;
  post_process_cout<double> ohlc_pp;
  moving_window_post_process_cout<microsecond_t, double> mv_pp;
  ohlc_t ohlc;
  moving_window<double, atp::common::sampler::close<double> > mid, bid, ask,
                 timer; // timer is just for tracking time and calling trades
};

TEST(OhlcPrototype, OhlcUsage)
{
  using namespace atp::common;
  using namespace atp::common::callback;

  int bars = 60; // seconds
  int bar_interval = 10;
  int scan_seconds = bars * bar_interval * 5;

  marketdata_handler<MarketData> feed_handler1;

  // A typical trader class that contains ohlc and other indicators
  Id id;
  id.set_name("trader1");
  id.set_variant("test1");
  id.set_signal("AAPL.STK");
  id.set_label("ohlc");
  trader trader(id, bars, bar_interval);

  // bind all the handlers
  trader.bind(feed_handler1);

  message_processor::protobuf_handlers_map symbol_handlers1;
  symbol_handlers1.register_handler("AAPL.STK", feed_handler1);
  symbol_handlers1.register_handler(
      atp::log_reader::DATA_END,
      boost::bind(&stop_function, _1, _2, "End of Log"));
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
