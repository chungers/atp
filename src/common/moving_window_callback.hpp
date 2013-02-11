#ifndef ATP_COMMON_MOVING_WINDOW_CALLBACK_H_
#define ATP_COMMON_MOVING_WINDOW_CALLBACK_H_

#include "common.hpp"
#include "common/time_series.hpp"

namespace atp {
namespace common {
namespace callback {

template <typename T, typename V>
struct moving_window_post_process
{
  virtual void operator()(const size_t count, const time_series<T,V>& window)
  {
    UNUSED(count); UNUSED(window);
    // no-op
  }
};

} // callback

} // common
} // atp

#endif //ATP_COMMON_MOVING_WINDOW_CALLBACK_H_
