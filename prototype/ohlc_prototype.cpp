
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
using namespace atp::time_series;
using namespace atp::time_series::callback;
using namespace atp::time_series::sampler;

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
namespace time_series {
namespace callback {

using atp::time_series::data_series;
using atp::time_series::microsecond_t;


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
                         const data_series<microsecond_t, V>& open,
                         const data_series<microsecond_t, V>& high,
                         const data_series<microsecond_t, V>& low,
                         const data_series<microsecond_t, V>& close)
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
} // time_series
} // atp


typedef ohlc<double> ohlc_t;

struct trader
{
  typedef atp::platform::callback::update_event<double>::func updater;

  trader(const Id& id, int bars, int seconds_per_bar) :
      id(id),
      ohlc_pp(),
      ohlc(seconds(bars * seconds_per_bar),
           seconds(seconds_per_bar), 0.)
  {
    ohlc.set(id);
    ohlc.set(ohlc_pp);
  }

  /// receives update of LAST
  void operator()(const microsecond_t& t, const double& v)
  {
    ptime tt = atp::time::as_ptime(t);
    cout << atp::time::to_est(tt) << ","
         << id.name() << ","
         << id.variant() << ","
         << id.signal() << ","
         << "last" << ","
         << v << std::endl;
    ohlc(t, v);
  }

  Id id;
  post_process_cout<double> ohlc_pp;
  ohlc_t ohlc;
};

TEST(OhlcPrototype, OhlcUsage)
{
  using namespace atp::time_series;
  using namespace atp::time_series::callback;

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
  feed_handler1.bind("LAST", static_cast<trader::updater>(trader));

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

