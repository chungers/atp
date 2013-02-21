#ifndef ATP_COMMON_MOVING_WINDOW_CALLBACK_H_
#define ATP_COMMON_MOVING_WINDOW_CALLBACK_H_

#include "common.hpp"
#include "common/time_series.hpp"

namespace atp {
namespace common {
namespace callback {

using atp::common::Id;

template <typename T, typename V>
struct moving_window_post_process
{
  virtual ~moving_window_post_process() {}

  virtual void operator()(const size_t count,
                          const Id& id,
                          const time_series<T,V>& window) = 0;
};

} // callback

} // common
} // atp

#endif //ATP_COMMON_MOVING_WINDOW_CALLBACK_H_
