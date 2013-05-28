#ifndef ATP_TIME_SERIES_MOVING_WINDOW_SAMPLERS_H_
#define ATP_TIME_SERIES_MOVING_WINDOW_SAMPLERS_H_

#include <cmath>

#include "common.hpp"


namespace atp {
namespace time_series {
namespace sampler {

template <typename element_t>
struct latest
{
  inline const element_t operator()(const element_t& last,
                                    const element_t& now,
                                    bool new_period)
  {
    UNUSED(last); UNUSED(new_period);
    return now;
  }
};

template <typename element_t>
class avg
{
 public:
  avg() : count(0) {}

  inline const element_t operator()(const element_t& last,
                                    const element_t& now,
                                    bool new_period)
  {
    UNUSED(last);
    if (new_period) {
      count = 0;
      sum = now;
    }
    sum = (count == 0) ? sum = now : sum + now;
    return sum / static_cast<element_t>(++count);
  }

 private:
  size_t count;
  element_t sum;
};

template <typename element_t>
struct max
{
  inline const element_t operator()(const element_t& last,
                                    const element_t& now,
                                    bool new_period)
  {
    return (new_period) ? now : std::max(last, now);
  }
};

template <typename element_t>
struct min
{
  inline const element_t operator()(const element_t& last,
                                    const element_t& now,
                                    bool new_period)
  {
    return (new_period) ? now : std::min(last, now);
  }
};

template <typename element_t>
class open
{
 public:

  open() : init_(false) {}

  inline const element_t operator()(const element_t& last,
                                    const element_t& now,
                                    bool new_period)
  {
    UNUSED(new_period); UNUSED(last);

    if (new_period) { open_ = now; init_ = true; }
    if (!init_) { open_ = now; init_ = true; }
    return open_;
  }

 private:
  element_t open_;
  bool init_;
};

template <typename element_t>
struct close
{
  inline const element_t operator()(const element_t& last,
                                    const element_t& now,
                                    bool new_period)
  {
    UNUSED(last); UNUSED(new_period);
    return now;
  }
};

} // namespace sampler
} // time_series
} // atp

#endif //ATP_TIME_SERIES_MOVING_WINDOW_SAMPLERS_H_
