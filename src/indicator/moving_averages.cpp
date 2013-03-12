#include <ta-lib/ta_func.h>


#include "common.hpp"
#include "indicator/moving_averages.hpp"



using namespace proto::indicator;

using atp::common::microsecond_t;


namespace atp {
namespace indicator {

SMA::SMA(const SMAConfig& config) :
    config_(config)
{

}

double SMA::operator()(const microsecond_t& t,
                       const double* series,
                       const size_t len)
{
  UNUSED(t);
  TA_Real out[len];
  TA_Integer outIndex, outCount;

  int ret = TA_SMA(0,
                   len-1,
                   series,
                   config_.period(),
                   &outIndex,
                   &outCount,
                   &out[0]);
  return out[outCount - 1];
}


EMA::EMA(const EMAConfig& config) :
    config_(config)
{

}

double EMA::operator()(const microsecond_t& t,
                       const double* series,
                       const size_t len)
{
  UNUSED(t);
  TA_Real out[len];
  TA_Integer outIndex, outCount;

  int ret = TA_EMA(0,
                   len-1,
                   series,
                   config_.period(),
                   &outIndex,
                   &outCount,
                   &out[0]);
  return out[outCount - 1];
}


KAMA::KAMA(const KAMAConfig& config) :
    config_(config)
{

}

double KAMA::operator()(const microsecond_t& t,
                       const double* series,
                       const size_t len)
{
  UNUSED(t);
  TA_Real out[len];
  TA_Integer outIndex, outCount;

  int ret = TA_KAMA(0,
                    len-1,
                    series,
                    config_.period(),
                    &outIndex,
                    &outCount,
                    &out[0]);
  return out[outCount - 1];
}

} // indicator
} // atp
