#ifndef ATP_PLATFORM_INDICATOR_H_
#define ATP_PLATFORM_INDICATOR_H_

#include "common/moving_window.hpp"
#include "common/moving_window_samplers.hpp"
#include "common/time_utils.hpp"

using atp::time_series::microsecond_t;
using atp::time_series::moving_window;
using atp::time_series::sample_interval_t;


namespace atp {
namespace platform {



/// Indicator is a moving window derived from another moving window.
template <typename V>
class indicator : public moving_window<V, atp::time_series::sampler::close<V> >
{
 public:
  indicator(time_duration h, sample_interval_t i, V init) :
      moving_window<V, atp::time_series::sampler::close<V> >(h, i, init)
  {
  }

  virtual const V calculate(const microsecond_t *t, const V *v,
                            const size_t len) = 0;
};


} // platform
} // atp

#endif //ATP_PLATFORM_INDICATOR_H_
