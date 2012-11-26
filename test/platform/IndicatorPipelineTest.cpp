

#include <string>

#include <boost/bind.hpp>

#include <gtest/gtest.h>
#include <glog/logging.h>


#include "platform/indicator.hpp"
#include "platform/marketdata_handler_proto_impl.hpp"
#include "platform/sequential_pipeline.hpp"


using std::string;


using namespace atp::time_series;
using namespace atp::platform;
using namespace atp::platform::marketdata;

using proto::ib::MarketData;
using proto::common::Value;


template <typename V>
class first_derivative : public indicator<V>
{
 public:
  first_derivative(const string& id,
                   time_duration h, sample_interval_t i, V init) :
      indicator<V>(h, i, init),
      id_(id)
  {
  }

  virtual const V calculate(const microsecond_t *t, const V *v,
                            const size_t len)
  {
    LOG(INFO) << id_ << ": calculate, len=" << len;
    return (v[len-1] - v[len-2]) / (t[len-1] - t[len-2]);
  }

 private:
  string id_;
};


TEST(IndicatorPipelineTest, UsageSyntax1)
{
  moving_window< double, atp::time_series::sampler::close<double> >
      price(microseconds(10), microseconds(1), 0);

  first_derivative<double> rate("rate", microseconds(10), microseconds(1), 0);

  sequential_pipeline<double, atp::time_series::sampler::close<double> >
      pp = price >> rate;

  microsecond_t t = now_micros();
  pp(t + 1, 10.);
  pp(t + 2, 10.);
  pp(t + 3, 10.);
  pp(t + 4, 40.);
  pp(t + 5, 50.);
  pp(t + 6, 60.);

  size_t len = 6;
  double vv[len];
  microsecond_t tt[len];

  LOG(INFO) << "price";
  EXPECT_EQ(len, price.copy_last(tt, vv, len));
  for (int i = 0; i < len; ++i) {
    LOG(INFO) << tt[i] << "," << vv[i];
  }

  LOG(INFO) << "d/dt";
  EXPECT_EQ(len, rate.copy_last(tt, vv, len));
  for (int i = 0; i < len; ++i) {
    LOG(INFO) << tt[i] << "," << vv[i];
  }
}


TEST(IndicatorPipelineTest, UsageSyntax2)
{

  marketdata_handler<MarketData> feed;

  moving_window< double, atp::time_series::sampler::close<double> >
      price(microseconds(10), microseconds(1), 0);

  first_derivative<double> rate("rate", microseconds(10), microseconds(1), 0);

  // bind to the data feed
  feed.bind("LAST", price >> rate);

  // send test messages
  microsecond_t start = now_micros();
  int count = 10;
  for (int i = 0; i < count; ++i) {

    MarketData md;
    md.set_timestamp(start + i);
    md.set_symbol("AAPL.STK");
    md.set_event("LAST");
    md.mutable_value()->set_type(proto::common::Value::DOUBLE);
    md.mutable_value()->set_double_value(600. + 10 * i);
    md.set_contract_id(12345);

    string message;
    EXPECT_TRUE(md.SerializeToString(&message));

    feed("AAPL.STK", message);
  }

  size_t len = count - 1; // don't care about the first point
  double vv[len];
  microsecond_t tt[len];

  LOG(INFO) << "price";
  EXPECT_EQ(len, price.copy_last(tt, vv, len));
  for (int i = 0; i < len; ++i) {
    LOG(INFO) << tt[i] << "," << vv[i];
  }

  LOG(INFO) << "d/dt";
  EXPECT_EQ(len, rate.copy_last(tt, vv, len));
  for (int i = 0; i < len; ++i) {
    LOG(INFO) << tt[i] << "," << vv[i];
  }
}
