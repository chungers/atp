#ifndef ATP_TIME_SERIES_OHLC_H_
#define ATP_TIME_SERIES_OHLC_H_

#include "common/time_series.hpp"
#include "common/moving_window.hpp"

using boost::posix_time::time_duration;


namespace atp {
namespace time_series {


namespace callback {

template <typename V, size_t K = 0>
struct post_process
{
  typedef typename sampler<V>::open ohlc_open;
  typedef typename sampler<V>::close ohlc_close;
  typedef typename sampler<V>::min ohlc_low;
  typedef typename sampler<V>::max ohlc_high;

  size_t get_count() { return K; }
  void operator()(const size_t count,
                  const moving_window<V, ohlc_open>& open,
                  const moving_window<V, ohlc_high>& high,
                  const moving_window<V, ohlc_low>& low,
                  const moving_window<V, ohlc_close>& close);
};

} // callback



template <typename V, typename PostProcess = callback::post_process<V> >
class ohlc
{
 public:

  typedef typename sampler<V>::open ohlc_open;
  typedef moving_window<V, ohlc_open > mw_open;

  typedef typename sampler<V>::close ohlc_close;
  typedef moving_window<V, ohlc_close > mw_close;

  typedef typename sampler<V>::max ohlc_high;
  typedef moving_window<V, ohlc_high > mw_high;

  typedef typename sampler<V>::min ohlc_low;
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
