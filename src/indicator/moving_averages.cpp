#include <ta-lib/ta_func.h>


#include "common.hpp"
#include "indicator/moving_averages.hpp"



using namespace proto::indicator;

using atp::common::microsecond_t;


namespace atp {
namespace indicator {

MA::MA(const MAConfig& config) :
    config_(config)
{

}

double MA::operator()(const microsecond_t& t,
                      const double* series,
                      const size_t len)
{
  UNUSED(t);
  TA_Real out[len];
  TA_Integer outIndex, outCount;

  int ret = 0;
  switch (config_.type()) {
    case MAConfig::EXPONENTIAL:
      ret = TA_EMA(0,
                   len-1,
                   series,
                   config_.period(),
                   &outIndex,
                   &outCount,
                   &out[0]);
      break;
    case MAConfig::KAUFMAN_ADAPTIVE:
      ret = TA_KAMA(0,
                    len-1,
                    series,
                    config_.period(),
                    &outIndex,
                    &outCount,
                    &out[0]);
      break;
    case MAConfig::SIMPLE:
    default:
      ret = TA_SMA(0,
                   len-1,
                   series,
                   config_.period(),
                   &outIndex,
                   &outCount,
                   &out[0]);
      break;
  }
  return out[outCount - 1];
}


} // indicator
} // atp
