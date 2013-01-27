#ifndef ATP_TIME_SERIES_MOVING_WINDOW_CALLBACKS_H_
#define ATP_TIME_SERIES_MOVING_WINDOW_CALLBACKS_H_

#include <string>

#include "common.hpp"
#include "common/moving_window.hpp"
#include "common/time_utils.hpp"



using namespace std;
using namespace boost::posix_time;
using atp::time_series::data_series;

/// Contains actual implementation of the callbacks as
/// defined in common/moving_window_callback.hpp

namespace atp {
namespace time_series {
namespace callback {

template <typename label_functor, typename T, typename V>
class moving_window_post_process_cout :
      public moving_window_post_process<T, V>
{
 public:

  moving_window_post_process_cout() :
      facet_(new time_facet("%Y-%m-%d %H:%M:%S%F%Q"))

  {
    std::cout.imbue(std::locale(std::cout.getloc(), facet_));
  }


  inline virtual void operator()(const size_t count,
                                 const data_series<T, V>& window)
  {
    UNUSED(count);
    // Writes to stdout the last stable sample
    ptime t = atp::time::as_ptime(window.get_time(-1));
    cout << atp::time::to_est(t) << ","
         << label_() << ","
         << window[-1] << endl;
  }

 private:
  time_facet* facet_;
  label_functor label_;
};

} // callback
} // time_series
} // atp

#endif //ATP_TIME_SERIES_MOVING_WINDOW_CALLBACKS_H_
