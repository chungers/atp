
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

#include "platform/marketdata_handler.hpp"
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

template <typename element_t>
struct closing
{
    inline const element_t operator()(const element_t& last,
                                      const element_t& now,
                                      bool new_period)
    {
      return now;
    }
};

template <typename V>
class indicator : public moving_window<V, closing<V> >
{
 public:
  indicator(time_duration h, sample_interval_t i, V init) :
      moving_window<V, closing<V> >(h, i, init)
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
size_t call_derived(moving_window< V, S>& source, indicator<V>& derived,
                      const microsecond_t& ts, const V& v, size_t len)
{
  size_t pushes = source.on(ts, v);
  // copy the data
  V vbuffer[len];
  microsecond_t tbuffer[len];
  size_t copied = source.copy_last(tbuffer, vbuffer, len);
  // calculate
  V computed = derived.calculate(tbuffer, vbuffer, len);
  return derived.on(ts, computed);
}

template <typename S, typename V>
function<size_t(const microsecond_t& ts, const V&)>
operator>>(moving_window<V, S>& source, indicator<V>& derived)
{
  return boost::bind(&call_derived<S, V>, boost::cref(source), boost::cref(derived),
                     _1, _2, 10);
}

} // prototype

TEST(NumericPrototype, DerivedCalculations)
{
  using namespace atp::time_series;
  using namespace atp::time_series::callback;

  moving_window< double, atp::time_series::sampler::close<double> >
      price(seconds(10), seconds(1), 0);

  // what's the proper data structure for something that depends
  // on another moving window?  ie. its value is not computed until
  // the source moving window is done.
}


