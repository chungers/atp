
#include <cmath>
#include <vector>
#include <boost/assign/std/vector.hpp>


#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include <glog/logging.h>

#include "utils.hpp"
#include "common/moving_window_callbacks.hpp"
#include "common/time_utils.hpp"

using namespace boost::assign;
using namespace boost::posix_time;

using namespace atp::time_series;



TEST(TimeSeriesTest, SamplerTest)
{
  atp::time_series::sampler::open<int> open;
  EXPECT_EQ(1, open(0, 1, true));
  EXPECT_EQ(1, open(1, 2, false));
  EXPECT_EQ(1, open(2, 3, false));
  EXPECT_EQ(1, open(3, 4, false));
  EXPECT_EQ(1, open(4, 5, false));
  EXPECT_EQ(6, open(5, 6, true));

  atp::time_series::sampler::close<int> close;
  EXPECT_EQ(0, close(-1, 0, false));
  EXPECT_EQ(1, close(0, 1, true));
  EXPECT_EQ(2, close(1, 2, false));
  EXPECT_EQ(3, close(2, 3, false));
  EXPECT_EQ(4, close(3, 4, false));
  EXPECT_EQ(5, close(4, 5, false));
  EXPECT_EQ(6, close(5, 6, true));

  atp::time_series::sampler::max<int> max;
  EXPECT_EQ(2, max(1, 2, false));
  EXPECT_EQ(3, max(2, 3, true));
  EXPECT_EQ(3, max(3, 2, false));
  EXPECT_EQ(5, max(2, 5, false));
  EXPECT_EQ(1, max(5, 1, true));

  atp::time_series::sampler::min<int> min;
  EXPECT_EQ(1, min(1, 2, false));
  EXPECT_EQ(3, min(2, 3, true));
  EXPECT_EQ(2, min(3, 2, false));
  EXPECT_EQ(2, min(2, 5, false));
  EXPECT_EQ(1, min(5, 1, true));

  atp::time_series::sampler::latest<int> latest;
  EXPECT_EQ(1, latest(0, 1, true));
  EXPECT_EQ(2, latest(1, 2, false));
  EXPECT_EQ(3, latest(2, 3, false));
  EXPECT_EQ(4, latest(3, 4, false));
  EXPECT_EQ(5, latest(4, 5, false));
  EXPECT_EQ(6, latest(5, 6, true));

  atp::time_series::sampler::avg<double> avg;
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


