
#include <vector>
#include <boost/assign/std/vector.hpp>


#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include <glog/logging.h>

#include "utils.hpp"
#include "common/moving_window.hpp"

using namespace boost::assign;
using namespace boost::posix_time;

using namespace atp::time_series;



TEST(TimeSeriesTest, SamplerTest)
{
  atp::time_series::sampler<int>::open open;
  EXPECT_EQ(1, open(0, 1, true));
  EXPECT_EQ(1, open(1, 2, false));
  EXPECT_EQ(1, open(2, 3, false));
  EXPECT_EQ(1, open(3, 4, false));
  EXPECT_EQ(1, open(4, 5, false));
  EXPECT_EQ(6, open(5, 6, true));

  atp::time_series::sampler<int>::close close;
  EXPECT_EQ(0, close(-1, 0, false));
  EXPECT_EQ(1, close(0, 1, true));
  EXPECT_EQ(2, close(1, 2, false));
  EXPECT_EQ(3, close(2, 3, false));
  EXPECT_EQ(4, close(3, 4, false));
  EXPECT_EQ(5, close(4, 5, false));
  EXPECT_EQ(6, close(5, 6, true));

  atp::time_series::sampler<int>::max max;
  EXPECT_EQ(2, max(1, 2, false));
  EXPECT_EQ(3, max(2, 3, true));
  EXPECT_EQ(3, max(3, 2, false));
  EXPECT_EQ(5, max(2, 5, false));
  EXPECT_EQ(1, max(5, 1, true));

  atp::time_series::sampler<int>::min min;
  EXPECT_EQ(1, min(1, 2, false));
  EXPECT_EQ(3, min(2, 3, true));
  EXPECT_EQ(2, min(3, 2, false));
  EXPECT_EQ(2, min(2, 5, false));
  EXPECT_EQ(1, min(5, 1, true));

  atp::time_series::sampler<int>::latest latest;
  EXPECT_EQ(1, latest(0, 1, true));
  EXPECT_EQ(2, latest(1, 2, false));
  EXPECT_EQ(3, latest(2, 3, false));
  EXPECT_EQ(4, latest(3, 4, false));
  EXPECT_EQ(5, latest(4, 5, false));
  EXPECT_EQ(6, latest(5, 6, true));

  atp::time_series::sampler<double>::avg avg;
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

TEST(TimeSeriesTest, MovingWindowPolicyTest)
{
  sample_interval_policy::align_at_zero p(10);
  EXPECT_EQ(10000, p.get_time(10001, 0));
  EXPECT_EQ(10000, p.get_time(10009, 0));
  EXPECT_EQ(10010, p.get_time(10010, 0));
  EXPECT_EQ(10000, p.get_time(10010, 1));
  EXPECT_EQ( 9990, p.get_time(10010, 2));
}

TEST(TimeSeriesTest, SampleOpenTest)
{
  using atp::time_series::sampler;
  moving_window< double, sampler<double>::open > open(
      microseconds(1000), microseconds(10), 0.);

  boost::uint64_t t = 10000000000;
  open.on(t + 10001, 10.);
  open.on(t + 10002, 12.);
  EXPECT_EQ(10., open[-1]);

  open.on(t + 10010, 13.);
  EXPECT_EQ(13., open[-1]);
  EXPECT_EQ(10., open[-2]);

  open.on(t + 10011, 14.);
  EXPECT_EQ(13., open[-1]);
  EXPECT_EQ(10., open[-2]);

  open.on(t + 10014, 15.);
  EXPECT_EQ(13., open[-1]);
  EXPECT_EQ(10., open[-2]);

  open.on(t + 10024, 16.);
  EXPECT_EQ(16., open[-1]);
  EXPECT_EQ(13., open[-2]);
  EXPECT_EQ(10., open[-3]);

  moving_window< double, sampler<double>::close > close(
      microseconds(1000), microseconds(10), 0.);

  close.on(t + 10001, 10.);
  close.on(t + 10002, 12.);
  EXPECT_EQ(12., close[-1]);

  close.on(t + 10010, 13.);
  EXPECT_EQ(13., close[-1]);
  EXPECT_EQ(12., close[-2]);

  close.on(t + 10011, 14.);
  EXPECT_EQ(14., close[-1]);
  EXPECT_EQ(12., close[-2]);

  close.on(t + 10014, 15.);
  EXPECT_EQ(15., close[-1]);
  EXPECT_EQ(12., close[-2]);

  close.on(t + 10024, 16.);
  EXPECT_EQ(16., close[-1]);
  EXPECT_EQ(15., close[-2]);
  EXPECT_EQ(12., close[-3]);

}

TEST(TimeSeriesTest, MovingWindowUsage)
{
  using atp::time_series::sampler;
  moving_window< double, sampler<double>::latest > last_trade(
      microseconds(1000), microseconds(10), 0.);

  boost::uint64_t t = 10000000000;

  last_trade.on(t + 10001, 10.); // 10.
  last_trade.on(t + 10011, 11.);
  last_trade.on(t + 10012, 12.); // 12.
  last_trade.on(t + 10022, 13.);
  last_trade.on(t + 10029, 15.);
  last_trade.on(t + 10029, 17.); // 17.
  last_trade.on(t + 10030, 20.);
  last_trade.on(t + 10031, 21.); // 21., 21., 21., 21.,
  last_trade.on(t + 10072, 22.);
  last_trade.on(t + 10074, 25.); // 25., 25.
  last_trade.on(t + 10095, 20.); // 20.
  last_trade.on(t + 10101, 24.);
  last_trade.on(t + 10109, 26.);
  last_trade.on(t + 10149, 20.); // 26, 26, 26, 26, 20


  std::vector<double> p;
  p += 10., 12., 17., 21., 21., 21., 21., 25., 25., 20.,
      26., 26., 26., 26., 20.;

  microsecond_t tbuff[p.size()];
  double buff[p.size()];

  int copied = last_trade.copy_last(tbuff, buff, p.size());

  EXPECT_EQ(p.size(), copied);

  for (int i = 0; i < p.size(); ++i) {
    LOG(INFO) << "(" << tbuff[i] << ", " << buff[i] << ")";
    EXPECT_EQ(p[i], buff[i]);
    EXPECT_EQ(t + 10000 + i * 10, tbuff[i]);
  }

  EXPECT_EQ(20., last_trade[-1]);
  EXPECT_EQ(26., last_trade[-2]);
  EXPECT_EQ(26., last_trade[-3]);

  last_trade.on(t + 10155, 27.); // 26, 26, 26, 26, 20, 27
  EXPECT_EQ(27., last_trade[-1]);
  EXPECT_EQ(20., last_trade[-2]);
  EXPECT_EQ(26., last_trade[-3]);

  last_trade.on(t + 10158, 29.); // 26, 26, 26, 26, 20, 29
  EXPECT_EQ(29., last_trade[-1]);
  EXPECT_EQ(20., last_trade[-2]);
  EXPECT_EQ(26., last_trade[-3]);

  last_trade.on(t + 10161, 31.); // 26, 26, 26, 26, 20, 29, 31
  EXPECT_EQ(31., last_trade[-1]);
  EXPECT_EQ(29., last_trade[-2]);
  EXPECT_EQ(20., last_trade[-3]);
  EXPECT_EQ(26., last_trade[-4]);

}

