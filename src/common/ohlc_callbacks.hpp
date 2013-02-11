#ifndef ATP_COMMON_OHLC_CALLBACKS_H_
#define ATP_COMMON_OHLC_CALLBACKS_H_

#include <string>

#include "common/moving_window_samplers.hpp"
#include "common/ohlc.hpp"
#include "common/time_utils.hpp"



using namespace std;
using namespace boost::posix_time;
using atp::common::time_series;


namespace atp {
namespace common {
namespace callback {

using atp::common::microsecond_t;

// partial specialization of the template
template <typename V>
class post_process_cout : public ohlc_post_process<V>
{
 public:

  typedef atp::common::sampler::open<V> ohlc_open;
  typedef atp::common::sampler::close<V> ohlc_close;
  typedef atp::common::sampler::min<V> ohlc_low;
  typedef atp::common::sampler::max<V> ohlc_high;

  post_process_cout() :
      facet_(new time_facet("%Y-%m-%d %H:%M:%S%F%Q"))
  {
    std::cout.imbue(std::locale(std::cout.getloc(), facet_));
  }

  inline void operator()(const size_t count,
                         const Id& id,
                         const time_series<microsecond_t, V>& open,
                         const time_series<microsecond_t, V>& high,
                         const time_series<microsecond_t, V>& low,
                         const time_series<microsecond_t, V>& close)
  {
    for (int i = -count; i < 0; ++i) {
      ptime t = atp::time::as_ptime(open.get_time(i));
      cout << atp::time::to_est(t) << ","
           << id << ","
           << open[i] << ","
           << high[i] << ","
           << low[i] << ","
           << close[i] << endl;
    }
  }

 private:
  time_facet* facet_;
};

} // callback
} // common
} // atp

#endif //ATP_COMMON_OHLC_CALLBACKS_H_
