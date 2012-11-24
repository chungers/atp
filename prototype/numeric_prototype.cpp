
#include <string>
#include <math.h>
#include <vector>

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <boost/thread.hpp>


#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include <glog/logging.h>

#include "common/moving_window.hpp"
#include "common/ohlc.hpp"
#include "common/ohlc_callbacks.hpp"
#include "common/time_utils.hpp"

#include "platform/marketdata_handler_proto_impl.hpp"
#include "platform/message_processor.hpp"

#include "proto/ib.pb.h"


using std::string;

using namespace boost::posix_time;
using namespace atp::time_series;
using namespace atp::time_series::callback;
using namespace atp::platform::marketdata;
using boost::posix_time::time_duration;

using proto::ib::MarketData;

using boost::function;


namespace prototype {

template <typename V>
class indicator : public moving_window<V, atp::time_series::sampler::close<V> >
{
 public:
  indicator(const string& id,
            time_duration h, sample_interval_t i, V init) :
      moving_window<V, atp::time_series::sampler::close<V> >(h, i, init),
      id_(id)
  {
    LOG(INFO) << get_id() << " created.";
  }

  virtual const V calculate(microsecond_t t[], V v[], const size_t len) = 0;

  const string& get_id() const
  {
    return id_;
  }

 private:
  string id_;
};

template <typename V>
class first_derivative : public indicator<V>
{
 public:
  first_derivative(const string& id,
                   time_duration h, sample_interval_t i, V init) :
      indicator<V>(id, h, i, init)
  {
  }

  virtual const V calculate(microsecond_t t[], V v[], const size_t len)
  {
    LOG(INFO) << indicator<V>::get_id() << ": calculate, len=" << len;
    return (v[len-1] - v[len-2]) / (t[len-1] - t[len-2]);
  }

};

template <typename V, typename S>
struct pipeline
{
  typedef typename boost::reference_wrapper< indicator< V > > indicator_ref;
  typedef typename std::vector< indicator_ref > indicator_list;
  typedef typename std::vector< indicator_ref >::iterator indicator_list_itr;

  pipeline(moving_window<V, S>& source) :
      source(boost::ref(source))
  {

  }

  void operator()(const microsecond_t& t, const V& v)
  {
    size_t len = 10;
    size_t pushes = source.get_pointer()->on(t, v);
    // copy the data
    V vbuffer[len];
    microsecond_t tbuffer[len];
    size_t copied = source.get_pointer()->copy_last(tbuffer, vbuffer, len);

    indicator_list_itr itr;
    for(itr = list.begin(); itr != list.end(); ++itr) {
      V computed = itr->get_pointer()->calculate(tbuffer, vbuffer, len);
      itr->get_pointer()->on(t, computed);
    }
  }

  void add(indicator<V>& derived)
  {
    list.push_back(boost::ref(derived));
  }

  pipeline<V, S>& operator>>(indicator<V>& derived)
  {
    add(derived);
    return *this;
  }

  boost::reference_wrapper<moving_window<V, S> > source;
  indicator_list list;
};

template <typename S, typename V>
pipeline<V, S>& operator>>(moving_window<V, S>& source, indicator<V>& derived)
{
  pipeline<V,S>* p = new pipeline<V,S>(source);
  p->add(derived);
  return *p;
}

} // prototype

namespace atp {
namespace platform {

template <typename E>
class prototype_marketdata_handler : public marketdata_handler<E>
{
 public:

  template <typename V, typename S>
  void bind(const string& event_code,
            prototype::pipeline<V,S>& pipeline);

  template <typename S> void bind(const string& event_code,
                                  prototype::pipeline<double,S>& pipeline)
  {
    callback::update_event<double>::func f = pipeline;
    marketdata_handler<E>::bind(event_code, f);
  }

  template <typename S> void bind(const string& event_code,
                                  prototype::pipeline<int,S>& pipeline)
  {
    callback::update_event<int>::func f = pipeline;
    marketdata_handler<E>::bind(event_code, f);
  }

  template <typename S> void bind(const string& event_code,
                                  prototype::pipeline<string,S>& pipeline)
  {
    callback::update_event<string>::func f = pipeline;
    marketdata_handler<E>::bind(event_code, f);
  }

};


} // platform
} // atp

TEST(NumericPrototype, DerivedCalculations)
{
  using namespace atp::time_series;
  using namespace atp::time_series::callback;
  using namespace prototype;

  atp::platform::prototype_marketdata_handler<MarketData> feed;

  moving_window< double, atp::time_series::sampler::close<double> >
      price(microseconds(10), microseconds(1), 0);

  first_derivative<double> close_rate("d/dt",
                                      microseconds(10), microseconds(1), 0);
  first_derivative<double> close_rate2("d/dt-2",
                                       microseconds(10), microseconds(1), 0);

  prototype::pipeline<double, atp::time_series::sampler::close<double> >
      pp = price >> close_rate2 >> close_rate;

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
  EXPECT_EQ(len, close_rate.copy_last(tt, vv, len));
  for (int i = 0; i < len; ++i) {
    LOG(INFO) << tt[i] << "," << vv[i];
  }

  LOG(INFO) << "d/dt-2";
  EXPECT_EQ(len, close_rate2.copy_last(tt, vv, len));
  for (int i = 0; i < len; ++i) {
    LOG(INFO) << tt[i] << "," << vv[i];
  }

  // bind to the data feed
  feed.bind("LAST", price >> close_rate >> close_rate2);

  // send a test message
  MarketData md;
  md.set_timestamp(now_micros());
  md.set_symbol("AAPL.STK");
  md.set_event("LAST");
  md.mutable_value()->set_type(proto::common::Value::DOUBLE);
  md.mutable_value()->set_double_value(600.);
  md.set_contract_id(12345);

  string message;
  EXPECT_TRUE(md.SerializeToString(&message));

  feed("AAPL.STK", message);

}


