#ifndef ATP_TIME_SERIES_OHLC_CALLBACKS_H_
#define ATP_TIME_SERIES_OHLC_CALLBACKS_H_


#include "common/ohlc.hpp"
#include "common/time_utils.hpp"

using namespace std;
using namespace boost::posix_time;

namespace atp {
namespace time_series {
namespace callback {


// partial specialization of the template
template <typename V>
class post_process_cout : public post_process<V>
{
 public:

  typedef typename sampler<V>::open ohlc_open;
  typedef typename sampler<V>::close ohlc_close;
  typedef typename sampler<V>::min ohlc_low;
  typedef typename sampler<V>::max ohlc_high;

  post_process_cout() :
      facet_(new time_facet("%Y-%m-%d %H:%M:%S%F%Q"))
  {
    std::cout.imbue(std::locale(std::cout.getloc(), facet_));
  }

  inline void operator()(const size_t count,
                         const moving_window<V, ohlc_open>& open,
                         const moving_window<V, ohlc_high>& high,
                         const moving_window<V, ohlc_low>& low,
                         const moving_window<V, ohlc_close>& close)
  {
    for (int i = -count; i < 0; ++i) {
      ptime t = atp::time::as_ptime(open.get_time(-2 + i));
      cout << atp::time::to_est(t) << "," << "open," << open[-2 + i] << endl;
      cout << atp::time::to_est(t) << "," << "high," << high[-2 + i] << endl;
      cout << atp::time::to_est(t) << "," << "low," << low[-2 + i] << endl;
      cout << atp::time::to_est(t) << "," << "close," << close[-2 + i] << endl;
    }
  }

 private:
  time_facet* facet_;
};

} // callback
} // time_series
} // atp

#endif ATP_TIME_SERIES_OHLC_CALLBACKS_H_
