#ifndef ATP_TIME_SERIES_MOVING_WINDOW_CALLBACKS_H_
#define ATP_TIME_SERIES_MOVING_WINDOW_CALLBACKS_H_

#include <string>

#include "common/moving_window.hpp"
#include "common/time_utils.hpp"



using namespace std;
using namespace boost::posix_time;
using atp::time_series::data_series;

namespace atp {
namespace time_series {
namespace callback {

template <typename label_functor, typename V>
class moving_window_post_process_cout :
      public moving_window_post_process<V>
{
 public:

  moving_window_post_process_cout() :
      facet_(new time_facet("%Y-%m-%d %H:%M:%S%F%Q"))

  {
    std::cout.imbue(std::locale(std::cout.getloc(), facet_));
  }


  inline void operator()(const size_t count, const data_series<V>& window)
  {
    for (int i = 1; i <= count; ++i) {
      ptime t = atp::time::as_ptime(window.get_time(-i));
      cout << atp::time::to_est(t) << ","
           << label_() << ","
           << window[-i] << endl;
    }
  }

 private:
  time_facet* facet_;
  label_functor label_;
};

} // callback
} // time_series
} // atp

#endif //ATP_TIME_SERIES_MOVING_WINDOW_CALLBACKS_H_
