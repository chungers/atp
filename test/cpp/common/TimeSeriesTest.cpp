
#include <cmath>
#include <vector>
#include <boost/assign/std/vector.hpp>


#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include <glog/logging.h>

#include "utils.hpp"
#include "common/ohlc.hpp"
#include "common/moving_window_callbacks.hpp"
#include "common/time_series.hpp"
#include "common/time_utils.hpp"

using namespace boost::assign;
using namespace boost::posix_time;

using atp::common::time_series;
using atp::common::microsecond_t;


TEST(TimeSeriesTest, SamplerTest)
{
  atp::common::sampler::open<int> open;
  EXPECT_EQ(1, open(0, 1, true));
  EXPECT_EQ(1, open(1, 2, false));
  EXPECT_EQ(1, open(2, 3, false));
  EXPECT_EQ(1, open(3, 4, false));
  EXPECT_EQ(1, open(4, 5, false));
  EXPECT_EQ(6, open(5, 6, true));

  atp::common::sampler::close<int> close;
  EXPECT_EQ(0, close(-1, 0, false));
  EXPECT_EQ(1, close(0, 1, true));
  EXPECT_EQ(2, close(1, 2, false));
  EXPECT_EQ(3, close(2, 3, false));
  EXPECT_EQ(4, close(3, 4, false));
  EXPECT_EQ(5, close(4, 5, false));
  EXPECT_EQ(6, close(5, 6, true));

  atp::common::sampler::max<int> max;
  EXPECT_EQ(2, max(1, 2, false));
  EXPECT_EQ(3, max(2, 3, true));
  EXPECT_EQ(3, max(3, 2, false));
  EXPECT_EQ(5, max(2, 5, false));
  EXPECT_EQ(1, max(5, 1, true));

  atp::common::sampler::min<int> min;
  EXPECT_EQ(1, min(1, 2, false));
  EXPECT_EQ(3, min(2, 3, true));
  EXPECT_EQ(2, min(3, 2, false));
  EXPECT_EQ(2, min(2, 5, false));
  EXPECT_EQ(1, min(5, 1, true));

  atp::common::sampler::latest<int> latest;
  EXPECT_EQ(1, latest(0, 1, true));
  EXPECT_EQ(2, latest(1, 2, false));
  EXPECT_EQ(3, latest(2, 3, false));
  EXPECT_EQ(4, latest(3, 4, false));
  EXPECT_EQ(5, latest(4, 5, false));
  EXPECT_EQ(6, latest(5, 6, true));

  atp::common::sampler::avg<double> avg;
  EXPECT_EQ(1.,
            avg(0., 1., true));
  EXPECT_EQ((1. + 2.) / 2.,
            avg(1., 2., false));
  EXPECT_EQ((1. + 2. + 3.) / 3.,
            avg(2., 3., false));
  EXPECT_EQ((1. + 2. + 3. + 4.) / 4.,
            avg(3., 4., false));
  EXPECT_EQ(5.,
            avg(4., 5., true));
}

using atp::common::ohlc;
using atp::common::moving_window;
using atp::common::time_series;


TEST(TimeSeriesTest, OhlcUsage)
{
  typedef moving_window<double, latest<double> > mw;
  typedef time_series<microsecond_t, double> series;
  typedef std::pair<microsecond_t, int> sample;
  typedef std::vector<sample> samples;


  unsigned int period_duration = 10;
  unsigned int periods = 10000;

  ohlc<double> fx(
      microseconds(period_duration * periods),
      microseconds(period_duration), 0.);

  vector<microsecond_t> times;
  samples data;

  microsecond_t t = now_micros();
  t = t - ( t % period_duration ); // this is so that time lines up nicely.

  microsecond_t now = now_micros();
  for (int i = 0; i <= periods*period_duration; ++i) {
    //double val = pow(static_cast<double>(i), 2.);

    double val = static_cast<double>(i);

    microsecond_t now2 = now_micros();
    fx(t + i, val);
    times.push_back(now_micros() - now2);
    data.push_back(sample(t + i, val));
  }

  microsecond_t sum = 0;;
  for (int i = 0; i < times.size(); ++i) {
    sum += times[i];
  }
  LOG(INFO) << "avg = " << sum/times.size();

  int len = 20;
  for (int i = fx.size(); i > (fx.size() - len); --i) {
    int j = 1-i;

    LOG(INFO) << "t = " << fx.open().get_time(j) - t
              << ", open  = " << fx.open()[j]
              << ", high  = " << fx.high()[j]
              << ", low   = " << fx.low()[j]
              << ", close = " << fx.close()[j];

  }

}
