#ifndef ATP_TIME_SERIES_OHLC_IMPL_H_
#define ATP_TIME_SERIES_OHLC_IMPL_H_

#include <glog/logging.h>


#include "common/ohlc.hpp"
#include "common/time_utils.hpp"


namespace atp {
namespace time_series {
namespace callback {


// partial specialization of the template
template <typename V>
struct post_process<V, 1>
{
  typedef typename sampler<V>::open ohlc_open;
  typedef typename sampler<V>::close ohlc_close;
  typedef typename sampler<V>::min ohlc_low;
  typedef typename sampler<V>::max ohlc_high;

  typedef moving_window< V, ohlc_open > mw_open;
  size_t get_count() { return 1; }
  void operator()(const size_t count,
                  const moving_window<V, ohlc_open>& open,
                  const moving_window<V, ohlc_high>& high,
                  const moving_window<V, ohlc_low>& low,
                  const moving_window<V, ohlc_close>& close)
  {
    for (int i = -count; i < 0; ++i) {
      ptime t = atp::time::as_ptime(open.get_time(-2 + i));
      LOG(INFO) << atp::time::to_est(t) << ","
                << "open=" << open[-2 + i]
                << ",high=" << high[-2 + i]
                << ",low=" << low[-2 + i]
                << ",close=" << close[-2 + i];
    }
  }
};

} // callback
} // time_series
} // atp

#endif ATP_TIME_SERIES_OHLC_IMPL_H_
