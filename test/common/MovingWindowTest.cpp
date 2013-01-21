
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



TEST(MovingWindowTest, MovingWindowPolicyTest)
{
  sample_interval_policy::align_at_zero p(10);
  EXPECT_EQ(10000, p.get_time(10001, 0));
  EXPECT_EQ(10000, p.get_time(10009, 0));
  EXPECT_EQ(10010, p.get_time(10010, 0));
  EXPECT_EQ(10000, p.get_time(10010, 1));
  EXPECT_EQ( 9990, p.get_time(10010, 2));

  EXPECT_TRUE(p.is_new_window(0, 10));
  EXPECT_FALSE(p.is_new_window(10, 11));
  EXPECT_FALSE(p.is_new_window(10, 12));
  EXPECT_FALSE(p.is_new_window(18, 19));
  EXPECT_TRUE(p.is_new_window(19, 20));

  sample_interval_policy::align_at_zero p2(1000000);// 1 sec.
  {
    microsecond_t t1 = 1349357412826693;
    microsecond_t t2 = 1349357414055175;
    ptime px1 = atp::time::as_ptime(t1);
    ptime px2 = atp::time::as_ptime(t2);
    int w = p2.count_windows(t1, t2);
    LOG(INFO) << "windows = " << w << ", diff = " << (t2 - t1) << ", "
              << atp::time::to_est(px1) << ", "
              << atp::time::to_est(px2);
    EXPECT_EQ(w, 2);
    EXPECT_TRUE(p2.is_new_window(t1, t2));

    EXPECT_EQ(1349357412000000, p2.get_time(t1, 0));
    EXPECT_EQ(1349357414000000, p2.get_time(t2, 0));
    EXPECT_EQ(1349357413000000, p2.get_time(t2, 1));
    EXPECT_EQ(1349357412000000, p2.get_time(t2, 2));
  }
  {
    microsecond_t t1 = 1349357414361850;
    microsecond_t t2 = 1349357415095778;

    int w = p2.count_windows(t1, t2);
    LOG(INFO) << "windows = " << w << ", diff = " << (t2 - t1) << ", "
              << atp::time::to_est(atp::time::as_ptime(t1)) << ", "
              << atp::time::to_est(atp::time::as_ptime(t2));
    EXPECT_EQ(w, 1);
    EXPECT_TRUE(p2.is_new_window(t1, t2));

    EXPECT_EQ(1349357414000000, p2.get_time(t1, 0));
    EXPECT_EQ(1349357415000000, p2.get_time(t2, 0));
    EXPECT_EQ(1349357414000000, p2.get_time(t2, 1));
  }
  {
    microsecond_t t1 = 1349357414361850;
    microsecond_t t2 = 1349357414361851;
    int w = p2.count_windows(t1, t2);
    LOG(INFO) << "windows = " << w << ", diff = " << (t2 - t1) << ", "
              << atp::time::to_est(atp::time::as_ptime(t1)) << ", "
              << atp::time::to_est(atp::time::as_ptime(t2));
    EXPECT_EQ(w, 0);
    EXPECT_FALSE(p2.is_new_window(t1, t2));

    EXPECT_EQ(1349357414000000, p2.get_time(t1, 0));
    EXPECT_EQ(1349357414000000, p2.get_time(t2, 0));
  }
}

TEST(MovingWindowTest, NegativeIndexTest1)
{
  microsecond_t n, dt;

  /// Test that adds more values than the buffer's capacity
  moving_window< double, sampler::close<double> > close1(
      microseconds(400), microseconds(1), 0.);

  for (int i = 0; i < 500; ++i) {
    close1(i, i);
    EXPECT_EQ(i, close1[0]); // current observation
    EXPECT_EQ(i, close1.get_time()); // current observation
  }
  n = now_micros();
  for (int i = 1; i < 400; ++i) {
    EXPECT_EQ(500 - 1 - i, close1[-i]);
  }
  dt = now_micros() - n;
  LOG(INFO) << "dt = " << dt;
}


TEST(MovingWindowTest, NegativeIndexTest2)
{
  microsecond_t n, dt;

  /// Test that adds exactly the buffer's capacity
  moving_window< double, sampler::close<double> > close2(
      microseconds(500), microseconds(1), 0.);

  for (int i = 0; i < 500; ++i) {
    close2(i, i);
    EXPECT_EQ(i, close2[0]); // current observation
    EXPECT_EQ(i, close2.get_time()); // current observation
  }

  EXPECT_EQ(499, close2[0]);
  EXPECT_EQ(499, close2.get_time());

  EXPECT_EQ(498, close2[-1]);
  EXPECT_EQ(498, close2.get_time(-1));

  n = now_micros();
  for (int i = 0; i < 500; ++i) {
    EXPECT_EQ(500 - 1 - i, close2[-i]);
  }
  dt = now_micros() - n;
  LOG(INFO) << "dt = " << dt;

  // now add values beyond the capacity of the buffer
  close2(500, 500);
  EXPECT_EQ(500, close2[0]);
  EXPECT_EQ(499, close2[-1]);
  EXPECT_EQ(500, close2.get_time());
  EXPECT_EQ(499, close2.get_time(-1));

  close2(501, 501);
  EXPECT_EQ(501, close2[0]);
  EXPECT_EQ(500, close2[-1]);
  EXPECT_EQ(501, close2.get_time());
  EXPECT_EQ(500, close2.get_time(-1));

}

TEST(MovingWindowTest, NegativeIndexTest3)
{
  microsecond_t n, dt;

  // the case where the entire buffer hasn't been filled.
  moving_window< double, sampler::close<double> > close3(
      microseconds(5000), microseconds(1), 0.);

  for (int i = 0; i < 500; ++i) {
    close3(i, i);
  }

  EXPECT_EQ(499, close3[0]);
  EXPECT_EQ(499, close3.get_time());

  EXPECT_EQ(498, close3[-1]);
  EXPECT_EQ(498, close3.get_time(-1));

  n = now_micros();
  for (int i = 1; i < 500; ++i) {
    EXPECT_EQ(499 - i, close3[-i]);
  }
  dt = now_micros() - n;
  LOG(INFO) << "dt = " << dt;
}

TEST(MovingWindowTest, SamplerTest)
{
  using namespace atp::time_series::sampler;
  moving_window< double, open<double> > open(
      microseconds(1000), microseconds(10), 0.);

  boost::uint64_t t = 10000000000;
  open.on(t + 10001, 10.);
  open.on(t + 10002, 12.);
  EXPECT_EQ(10., open[0]);
  EXPECT_EQ(0., open[-1]);

  open.on(t + 10010, 13.);
  EXPECT_EQ(13., open[0]);
  EXPECT_EQ(10., open[-1]);

  open.on(t + 10011, 14.);
  EXPECT_EQ(13., open[0]);
  EXPECT_EQ(10., open[-1]);

  open.on(t + 10014, 15.);
  EXPECT_EQ(13., open[0]);
  EXPECT_EQ(10., open[-1]);

  open.on(t + 10024, 16.);
  EXPECT_EQ(16., open[0]);
  EXPECT_EQ(13., open[-1]);
  EXPECT_EQ(10., open[-2]);

  moving_window< double, sampler::close<double> > close(
      microseconds(1000), microseconds(10), 0.);

  close.on(t + 10001, 10.);
  close.on(t + 10002, 12.);
  EXPECT_EQ(12., close[0]);

  close.on(t + 10010, 13.);
  EXPECT_EQ(13., close[0]);
  EXPECT_EQ(12., close[-1]);

  close.on(t + 10011, 14.);
  EXPECT_EQ(14., close[0]);
  EXPECT_EQ(12., close[-1]);

  close.on(t + 10014, 15.);
  EXPECT_EQ(15., close[0]);
  EXPECT_EQ(12., close[-1]);

  close.on(t + 10024, 16.);
  EXPECT_EQ(16., close[0]);
  EXPECT_EQ(15., close[-1]);
  EXPECT_EQ(12., close[-2]);

}


struct label {
  inline const string operator()() const { return "last"; }
};


TEST(MovingWindowTest, MovingWindowUsage)
{
  using namespace atp::time_series::sampler;


  typedef atp::time_series::callback::moving_window_post_process_cout<
    label, double > pp;

  moving_window<double, latest<double>, pp > last_trade(
      microseconds(1000), microseconds(10), 0.);

  boost::uint64_t ct, t = now_micros();
  t = t - ( t % 10 ); // this is so that time lines up nicely.

  last_trade.on(ct = t + 10001, 10.); // 10.

  EXPECT_EQ(t + 10000, last_trade.get_time());

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
  last_trade.on(ct = t + 10149, 20.); // 26, 26, 26, 26, 20

  EXPECT_EQ(t + 10140, last_trade.get_time());
  EXPECT_EQ(t + 10130, last_trade.get_time(-1));
  EXPECT_EQ(26., last_trade[-1]);

  std::vector<double> p;
  p += 10., 12., 17., 21., 21., 21., 21., 25., 25., 20.,
      26., 26., 26., 26., 20.;

  microsecond_t tbuff[p.size()];
  double buff[p.size()];

  int copied = last_trade.copy_last(&tbuff[0], &buff[0], p.size());

  EXPECT_EQ(p.size(), copied);

  for (int i = 0; i < p.size(); ++i) {
    LOG(INFO) << "(" << tbuff[i] << ", " << buff[i] << ")";
    EXPECT_EQ(p[i], buff[i]);
    EXPECT_EQ(t + 10000 + i * 10, tbuff[i]);
  }

  EXPECT_EQ(20., last_trade[0]);
  EXPECT_EQ(26., last_trade[-1]);
  EXPECT_EQ(26., last_trade[-2]);

  last_trade.on(t + 10155, 27.); // 26, 26, 26, 26, 20, 27
  EXPECT_EQ(27., last_trade[0]);
  EXPECT_EQ(20., last_trade[-1]);
  EXPECT_EQ(26., last_trade[-2]);

  last_trade(t + 10158, 29.); // 26, 26, 26, 26, 20, 29
  EXPECT_EQ(29., last_trade[0]);
  EXPECT_EQ(20., last_trade[-1]);
  EXPECT_EQ(26., last_trade[-2]);

  last_trade(t + 10161, 31.); // 26, 26, 26, 26, 20, 29, 31
  EXPECT_EQ(31., last_trade[0]);
  EXPECT_EQ(29., last_trade[-1]);
  EXPECT_EQ(20., last_trade[-2]);
  EXPECT_EQ(26., last_trade[-3]);

}

TEST(MovingWindowTest, FunctionTest)
{
  using namespace atp::time_series::sampler;


  typedef atp::time_series::callback::moving_window_post_process_cout<
    label, double > pp;

  moving_window<double, latest<double>, pp > fx(
      microseconds(40), microseconds(2), 0.);

  boost::uint64_t t = now_micros();
  t = t - ( t % 10 ); // this is so that time lines up nicely.

  for (int i = 0; i <= 40*2; ++i) {
    double val = pow(static_cast<double>(i), 2.);
    LOG(INFO) << atp::time::to_est(atp::time::as_ptime(t + i))
              <<", i = " << i << ", " << val;
    fx(t + i, val);
  }

  size_t len = 20;
  microsecond_t tbuff[len];
  double buff[len];

  int copied = fx.copy_last(&tbuff[0], &buff[0], len);

  EXPECT_EQ(len, copied);

  LOG(INFO) << "t = " << t;
  LOG(INFO) << "get_time[0] = " << fx.get_time() - t;
  LOG(INFO) << "current_t = " << fx.current_sample_time() - t;
  for (int i = 0; i < len; ++i) {
    LOG(INFO)
        << atp::time::to_est(atp::time::as_ptime(tbuff[i])) << ", "
        << "(" << tbuff[i] - t << ", " << buff[i] << ")";
  }

}
