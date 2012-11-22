#ifndef ATP_TIME_SERIES_OHLC_H_
#define ATP_TIME_SERIES_OHLC_H_

#include "common/time_series.hpp"
#include "common/moving_window.hpp"

using boost::posix_time::time_duration;


namespace atp {
namespace time_series {
namespace callback {

template <typename V>
struct ohlc_post_process
{
  typedef atp::time_series::sampler::open<V> ohlc_open;
  typedef atp::time_series::sampler::close<V> ohlc_close;
  typedef atp::time_series::sampler::min<V> ohlc_low;
  typedef atp::time_series::sampler::max<V> ohlc_high;

  inline void operator()(const size_t count,
                         const moving_window<V, ohlc_open>& open,
                         const moving_window<V, ohlc_high>& high,
                         const moving_window<V, ohlc_low>& low,
                         const moving_window<V, ohlc_close>& close)
  {
    // no-op
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
  {
  }

  size_t on(microsecond_t timestamp, V value)
  {
    size_t open = open_.on(timestamp, value);
    size_t close = close_.on(timestamp, value);
    size_t high = high_.on(timestamp, value);
    size_t low = low_.on(timestamp, value);

    // assume the sizes are all the same.
    post_(open, open_, high_, low_, close_);
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
  PostProcess post_;
};


} // time_series
} // atp


#endif //ATP_TIME_SERIES_OHLC_H_
