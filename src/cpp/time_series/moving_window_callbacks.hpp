#ifndef ATP_TIME_SERIES_MOVING_WINDOW_CALLBACKS_H_
#define ATP_TIME_SERIES_MOVING_WINDOW_CALLBACKS_H_

#include <string>

#include "common.hpp"
#include "time_series/moving_window.hpp"
#include "time_series/time_utils.hpp"



using namespace std;
using namespace boost::posix_time;
using atp::common::Id;
using atp::time_series::time_series;

/// Contains actual implementation of the callbacks as
/// defined in time_series/moving_window_callback.hpp

namespace atp {
namespace time_series {
namespace callback {



template <typename T, typename V>
struct moving_window_post_process_noop : public moving_window_post_process<T, V>
{
  virtual void operator()(const size_t count,
                          const Id& id,
                          const time_series<T,V>& window)
  {
    UNUSED(count); UNUSED(id); UNUSED(window);
  }
};

template <typename T, typename V>
class moving_window_post_process_cout : public moving_window_post_process<T, V>
{
 public:

  moving_window_post_process_cout() :
      facet_(new time_facet("%Y-%m-%d %H:%M:%S%F%Q"))

  {
    std::cout.imbue(std::locale(std::cout.getloc(), facet_));
  }


  inline virtual void operator()(const size_t count,
                                 const Id& id,
                                 const time_series<T, V>& window)
  {
    for (int i = -count; i < 0; ++i) {
      ptime t = atp::time_series::as_ptime(window.get_time(i));
      cout << atp::time_series::to_est(t) << ","
           << id << ","
           << window[i] << endl;
    }
  }

 private:
  time_facet* facet_;
};

} // callback
} // time_series
} // atp

#endif //ATP_TIME_SERIES_MOVING_WINDOW_CALLBACKS_H_
