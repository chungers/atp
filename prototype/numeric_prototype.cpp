
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
  indicator(time_duration h, sample_interval_t i, V init) :
      moving_window<V, atp::time_series::sampler::close<V> >(h, i, init)
  {
  }

  virtual const V calculate(microsecond_t t[], V v[], const size_t len) = 0;
};

template <typename V>
class first_derivative : public indicator<V>
{
 public:
  first_derivative(time_duration h, sample_interval_t i, V init) :
      indicator<V>(h, i, init)
  {
  }

  virtual const V calculate(microsecond_t t[], V v[], const size_t len)
  {
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
    bind(event_code, pipeline);
  }

  template <typename S> void bind(const string& event_code,
                                  prototype::pipeline<int,S>& pipeline)
  {
    bind(event_code, pipeline);
  }

  template <typename S> void bind(const string& event_code,
                                  prototype::pipeline<string,S>& pipeline)
  {
    bind(event_code, pipeline);
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
      price(seconds(10), seconds(1), 0);

  first_derivative<double> close_rate(seconds(10), seconds(1), 0);
  first_derivative<double> close_rate2(seconds(10), seconds(1), 0);

  price >> close_rate >> close_rate2;

  prototype::pipeline<double, atp::time_series::sampler::close<double> >
      pp(price);

  feed.bind("LAST", price >> close_rate >> close_rate2);

}


