#ifndef ATP_TIME_SERIES_MOVING_WINDOW_INTERVAL_POLICY_H_
#define ATP_TIME_SERIES_MOVING_WINDOW_INTERVAL_POLICY_H_

#include "time_series/time_utils.hpp"


namespace atp {
namespace time_series {

using atp::common::microsecond_t;

struct time_interval_policy {

  struct align_at_zero
  {

    align_at_zero(const microsecond_t& window_size)
        : window_size_(window_size)
    {
    }

    /// returns the sampled time instant -- aligned at zero
    inline microsecond_t get_time(const microsecond_t& curr_ts,
                                  const size_t& offset) const
    {
      microsecond_t r = curr_ts - (curr_ts % window_size_);
      return r - offset * window_size_;
    }

    inline int count_windows(const microsecond_t& last_ts,
                             const microsecond_t& timestamp)
    {
      if (timestamp < last_ts) return 0;

      microsecond_t m1 = last_ts - (last_ts % window_size_);
      microsecond_t m2 = timestamp - (timestamp % window_size_);
      return (m2 - m1) / window_size_;
    }

    inline bool is_new_window(const microsecond_t& last_ts,
                       const microsecond_t& timestamp)
    {
      return count_windows(last_ts, timestamp) > 0;
    }

    microsecond_t window_size_;
  };

};


} // time_series
} // atp

#endif // ATP_TIME_SERIES_MOVING_WINDOW_INTERVAL_POLICY_H_
