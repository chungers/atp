#ifndef ATP_TIME_SERIES_OHLC_H_
#define ATP_TIME_SERIES_OHLC_H_

#include "common.hpp"
#include "common/time_series.hpp"
#include "common/moving_window.hpp"
#include "common/moving_window_samplers.hpp"

using boost::posix_time::time_duration;
using atp::time_series::data_series;


namespace atp {
namespace time_series {
namespace callback {

using atp::time_series::microsecond_t;

template <typename V>
struct ohlc_post_process
{
  typedef atp::time_series::sampler::open<V> ohlc_open;
  typedef atp::time_series::sampler::close<V> ohlc_close;
  typedef atp::time_series::sampler::min<V> ohlc_low;
  typedef atp::time_series::sampler::max<V> ohlc_high;

  inline void operator()(const size_t count,
                         const data_series<microsecond_t, V>& open,
                         const data_series<microsecond_t, V>& high,
                         const data_series<microsecond_t, V>& low,
                         const data_series<microsecond_t, V>& close)
  {
    UNUSED(count); UNUSED(open); UNUSED(high); UNUSED(low); UNUSED(close);
  }
};
} // callback


template <typename V, typename PostProcess = callback::ohlc_post_process<V> >
class ohlc
{
 public:

  typedef atp::time_series::sampler::open<V> ohlc_open;
  typedef moving_window<V, ohlc_open > mw_open;

  typedef atp::time_series::sampler::close<V> ohlc_close;
  typedef moving_window<V, ohlc_close > mw_close;

  typedef atp::time_series::sampler::max<V> ohlc_high;
  typedef moving_window<V, ohlc_high > mw_high;

  typedef atp::time_series::sampler::min<V> ohlc_low;
  typedef moving_window<V, ohlc_low > mw_low;

  ohlc(time_duration h, time_duration s, V initial) :
      open_(h, s, initial)
      , close_(h, s, initial)
      , high_(h, s, initial)
      , low_(h, s, initial)
      , t(open_)
  {
  }

  size_t operator()(const microsecond_t& timestamp, const V& value)
  {
    return on(timestamp, value);
  }

  size_t size()
  {
    return open_.size(); // any of the components
  }

  size_t on(const microsecond_t& timestamp, const V& value)
  {
    size_t open = open_(timestamp, value);
    close_(timestamp, value);
    high_(timestamp, value);
    low_(timestamp, value);

    // assume the sizes are all the same.
    post_(open, open_, high_, low_, close_);
    return open;
  }

  const data_series<microsecond_t, V>& open() const
  {
    return open_;
  }

  const data_series<microsecond_t, V>& close() const
  {
    return close_;
  }

  const data_series<microsecond_t, V>& high() const
  {
    return high_;
  }

  const data_series<microsecond_t, V>& low() const
  {
    return low_;
  }

 public:
  const time_axis<microsecond_t, V> t;

 private:

  mw_open open_;
  mw_close close_;
  mw_high high_;
  mw_low low_;
  PostProcess post_;

};


} // time_series
} // atp


#endif //ATP_TIME_SERIES_OHLC_H_
