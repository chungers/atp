#ifndef ATP_TIME_SERIES_MOVING_WINDOW_CALLBACK_H_
#define ATP_TIME_SERIES_MOVING_WINDOW_CALLBACK_H_

#include "common.hpp"
#include "time_series/time_series.hpp"


namespace atp {
namespace time_series {
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

} // time_series
} // atp

#endif //ATP_TIME_SERIES_MOVING_WINDOW_CALLBACK_H_
