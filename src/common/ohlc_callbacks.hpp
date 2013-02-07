#ifndef ATP_TIME_SERIES_OHLC_CALLBACKS_H_
#define ATP_TIME_SERIES_OHLC_CALLBACKS_H_

#include <string>

#include "common/moving_window_samplers.hpp"
#include "common/ohlc.hpp"
#include "common/time_utils.hpp"



using namespace std;
using namespace boost::posix_time;
using atp::time_series::data_series;


namespace atp {
namespace time_series {
namespace callback {

using atp::time_series::microsecond_t;

// partial specialization of the template
template <typename V>
class post_process_cout : public ohlc_post_process<V>
{
 public:

  typedef atp::time_series::sampler::open<V> ohlc_open;
  typedef atp::time_series::sampler::close<V> ohlc_close;
  typedef atp::time_series::sampler::min<V> ohlc_low;
  typedef atp::time_series::sampler::max<V> ohlc_high;

  post_process_cout(const string& id, const string& label) :
      id_(id), label_(label),
      facet_(new time_facet("%Y-%m-%d %H:%M:%S%F%Q"))
  {
    std::cout.imbue(std::locale(std::cout.getloc(), facet_));
  }

  inline void operator()(const size_t count,
                         const data_series<microsecond_t, V>& open,
                         const data_series<microsecond_t, V>& high,
                         const data_series<microsecond_t, V>& low,
                         const data_series<microsecond_t, V>& close)
  {
    for (int i = -count; i < 0; ++i) {
      ptime t = atp::time::as_ptime(open.get_time(i));
      cout << atp::time::to_est(t) << ","
           << id_ << ","
           << label_ << ","
           << open[i] << ","
           << high[i] << ","
           << low[i] << ","
           << close[i] << endl;
    }
  }

 private:
  string id_;
  string label_;
  time_facet* facet_;
};

} // callback
} // time_series
} // atp

#endif //ATP_TIME_SERIES_OHLC_CALLBACKS_H_
