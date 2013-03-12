#ifndef ATP_INDICATOR_MOVING_AVERAGES_H_
#define ATP_INDICATOR_MOVING_AVERAGES_H_

#include "common/time_series.hpp"
#include "proto/indicator.pb.h"

using namespace proto::indicator;

using atp::common::microsecond_t;


namespace atp {
namespace indicator {

///
class MA
{
 public:

  MA(const MAConfig& config);

  double operator()(const microsecond_t& t,
                    const double* series,
                    const size_t len);

 private:
  MAConfig config_;
};


} // indicator
} // atp


#endif //ATP_INDICATOR_MOVING_AVERAGES_H_
