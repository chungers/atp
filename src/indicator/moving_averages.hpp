#ifndef ATP_INDICATOR_MOVING_AVERAGES_H_
#define ATP_INDICATOR_MOVING_AVERAGES_H_

#include "common/time_series.hpp"
#include "proto/indicator.pb.h"

using namespace proto::indicator;

using atp::common::microsecond_t;


namespace atp {
namespace indicator {

/// Simple Moving Average
class SMA
{
 public:

  SMA(const SMAConfig& config);

  double operator()(const microsecond_t& t,
                    const double* series,
                    const size_t len);

 private:
  SMAConfig config_;
};

/// Exponential Moving Average
class EMA
{
 public:

  EMA(const EMAConfig& config);

  double operator()(const microsecond_t& t,
                    const double* series,
                    const size_t len);

 private:
  EMAConfig config_;
};


/// Kaufman Adaptive Moving Average
class KAMA
{
 public:

  KAMA(const KAMAConfig& config);

  double operator()(const microsecond_t& t,
                    const double* series,
                    const size_t len);

 private:
  KAMAConfig config_;
};


} // indicator
} // atp


#endif //ATP_INDICATOR_MOVING_AVERAGES_H_
