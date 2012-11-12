
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



template <typename element_t>
struct last_trade
{
  element_t operator()(element_t last, element_t now, bool new_period)
  {
    return now;
  }
};

TEST(TimeSeriesTest, MovingWindowPolicyTest)
{
  sample_interval_policy::align_at_zero p(10);
  EXPECT_EQ(10000, p.get_time(10001, 0));
  EXPECT_EQ(10000, p.get_time(10009, 0));
  EXPECT_EQ(10010, p.get_time(10010, 0));
  EXPECT_EQ(10000, p.get_time(10010, 1));
  EXPECT_EQ( 9990, p.get_time(10010, 2));
}

TEST(TimeSeriesTest, MovingWindowUsage)
{
  moving_window< double, last_trade<double> > last_trade(
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

