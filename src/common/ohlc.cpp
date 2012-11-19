
#include <glog/logging.h>


#include "common/ohlc.hpp"
#include "common/time_utils.hpp"


namespace atp {
namespace time_series {
namespace callback {

/**
   Idea here is to include the specific implementation, but
   template specializations won't link properly unless it's inlcuded
   inline.
 */
template <typename V>
void post_process<V, 1>::operator()(const size_t count,
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

} // callback
} // time_series
} // atp
