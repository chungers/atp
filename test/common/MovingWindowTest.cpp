
#include <cmath>
#include <vector>
#include <iostream>

#include <boost/assign/std/vector.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/tuple/tuple.hpp>


#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include <glog/logging.h>

#include "utils.hpp"
#include "common/moving_window_callbacks.hpp"
#include "common/moving_window_samplers.hpp"
#include "common/moving_window.hpp"
#include "common/time_utils.hpp"
#include "common/types.hpp"

using namespace boost;
using namespace boost::assign;
using namespace boost::posix_time;
using namespace atp::common::sampler;

using atp::common::time_interval_policy;
using atp::common::moving_window;
using atp::common::time_series;
using atp::common::microsecond_t;




TEST(MovingWindowTest, MovingWindowPolicyTest)
{
  time_interval_policy::align_at_zero p(10);
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

  time_interval_policy::align_at_zero p2(1000000);// 1 sec.
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
  moving_window< double, close<double> > close1(
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
  moving_window< double, close<double> > close2(
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
  moving_window< double, close<double> > close3(
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

  moving_window< double, close<double> > close(
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
  typedef atp::common::callback::moving_window_post_process_cout<
    microsecond_t, double > pp;

  moving_window<double, latest<double> > last_trade(
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

  for (size_t i = 0; i < p.size(); ++i) {
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
  typedef atp::common::callback::moving_window_post_process_cout<
    microsecond_t, double > pp;

  typedef std::pair<microsecond_t, int> sample;
  typedef std::vector<sample> series;
  typedef series::iterator series_itr;
  typedef series::reverse_iterator series_reverse_itr;

  unsigned int period_duration = 3;
  unsigned int periods = 500;

  // NOTE the default here is closing value of the period and
  // is sampling at some weird duration us interval.
  // So this has the effect of shifting data backwards.
  // For example, at time period of 2 usec:
  // input = [0, 1], [1, 2], [2, 3], [3, 4], [4, 5]
  // moving_window = [0, 2], [2, 4], [4, 5]
  moving_window<double, latest<double> > fx(
      microseconds(period_duration * periods),
      microseconds(period_duration), 0.);

  boost::uint64_t t = now_micros();
  t = t - ( t % period_duration ); // this is so that time lines up nicely.

  series data, expects;
  for (unsigned int i = 0; i <= periods*period_duration; ++i) {
    double val = pow(static_cast<double>(i), 2.) - 10. * i;
    VLOG(100) << atp::time::to_est(atp::time::as_ptime(t + i))
              <<", i = " << i << ", " << val;
    fx(t + i, val);

    data.push_back(sample(t + i, val));
  }

  // Calculate expectations based on closing of a period to
  // match the moving_window's sampler (defaults to close)
  microsecond_t period_start = t;
  int period_close = 0;
  for (series_itr itr = data.begin(); itr != data.end(); ++itr) {
    if (itr->first - period_start >= period_duration) {
      expects.push_back(sample(period_start, period_close));
      period_start = itr->first;
    }
    // closing value of the period --> always the latest value
    period_close = itr->second;
  }
  // add last current observation
  expects.push_back(sample(period_start, period_close));

  VLOG(50) << "expectations:";
  for (series_itr itr = expects.begin(); itr != expects.end(); ++itr) {
    VLOG(50)
        << atp::time::to_est(atp::time::as_ptime(itr->first)) << ", "
        << "(" << itr->first - t << ", " << itr->second << ")";
  }

  size_t len = periods + 1;  // history + current observation
  microsecond_t tbuff[len];
  double buff[len];

  int copied = fx.copy_last(&tbuff[0], &buff[0], len);

  EXPECT_EQ(len, copied);

  LOG(INFO) << "t = " << t;
  LOG(INFO) << "get_time[0] = " << fx.get_time() - t;

  // Compare the sampled data with expectations
  for (unsigned int i = 0; i < len; ++i) {
    VLOG(50)
        << atp::time::to_est(atp::time::as_ptime(tbuff[i])) << ", "
        << "(" << tbuff[i] - t << ", " << buff[i] << ")";

    ASSERT_EQ(expects[i].first, tbuff[i]);
    ASSERT_EQ(expects[i].second, buff[i]);
  }

  /// Quick test of the data_series interface
  LOG(INFO) << "data_series";
  ASSERT_EQ(expects.size(), fx.size());

  // Checks the current observation
  series_reverse_itr expects_itr = expects.rbegin();
  ASSERT_EQ(expects_itr->first, fx.get_time(0));
  ASSERT_EQ(expects_itr->second, fx[0]);

  expects_itr++;

  // Past observations
  for (unsigned int i = 1; i < fx.size(); ++i, ++expects_itr) {
    microsecond_t tt = fx.get_time(-i);

    int v = fx[-i];
    VLOG(50)
        << atp::time::to_est(atp::time::as_ptime(tt)) << ", "
        << "(" << tt - t << ", " << v << ")";
    ASSERT_EQ(expects_itr->first, tt);
    ASSERT_EQ(expects_itr->second, v);
  }
}

namespace operations {

/// first derivative
struct dfdt
{
  double operator()(const time_series<microsecond_t, double>& series)
  {
    return (series[0] - series[-1]) /
        (series.get_time(0) - series.get_time(-1));
  }
};

/// second derivative
struct df2dt2
{
  double operator()(const time_series<microsecond_t, double>& series)
  {
    return (series[0] - 2. * series[-1] + series[-2]) /
        pow((series.get_time(0) - series.get_time(-1)), 2.);
  }
};

struct log
{
  double operator()(const time_series<microsecond_t, double>& series)
  {
    return ::log(series[0]);
  }
  double operator()(const double* series, const size_t len)
  {
    (void)(len);
    return ::log(series[0]);
  }
};

struct negate
{
  double operator()(const time_series<microsecond_t, double>& series)
  {
    return -series[0];
  }
};

struct linear
{
  linear(const double slope, const double intercept) :
      slope(slope), intercept(intercept) {}

  double operator()(const time_series<microsecond_t, double>& series)
  {
    return slope * series[0] + intercept;
  }

  double slope;
  double intercept;
};

} // operations

double get_average_time(vector<microsecond_t>& times)
{
  double avg = 0.;
  for (unsigned int i = 0; i < times.size(); ++i) {
    avg += static_cast<double>(times[i]) / times.size();
  }
  return avg;
}

TEST(MovingWindowTest, SeriesOperationsUsage)
{
  typedef atp::common::callback::moving_window_post_process_cout<
    microsecond_t, double > pp_stdout;

  typedef moving_window<double, latest<double> > close_series;
  typedef time_series<microsecond_t, double> trace;
  typedef std::pair<microsecond_t, int> sample;
  typedef std::vector<sample> series;
  typedef series::iterator series_itr;
  typedef series::reverse_iterator series_reverse_itr;

  unsigned int period_duration = 1;
  unsigned int periods = 500;

  close_series fx(
      microseconds(period_duration * periods),
      microseconds(period_duration), 0.);

  trace& log = fx.apply3("log", operations::log());
  trace& dxdt = fx.apply("dx/dt", operations::dfdt(), 2);
  trace& d2xdt2 = fx.apply("d2x/dt2", operations::df2dt2(), 3);

  // dependent the result of the computation --> an extra pipeline stage.
  trace& linear = fx.apply("2x+1", operations::linear(2., 1.));
  trace& dxdt_2 = linear.apply("dx/dt", operations::dfdt(), 2);

  boost::uint64_t t = now_micros();
  t = t - ( t % period_duration ); // this is so that time lines up nicely.

  series data, expects;

  vector<microsecond_t> times;

  microsecond_t now = now_micros();
  for (unsigned int i = 0; i <= periods*period_duration; ++i) {
    double val = pow(static_cast<double>(i), 2.);

    microsecond_t now2 = now_micros();
    fx(t + i, val);
    times.push_back(now_micros() - now2);
    data.push_back(sample(t + i, val));
  }
  LOG(INFO) << "push/ compute dt = " << (now_micros() - now);
  LOG(INFO) << "avg = " << get_average_time(times);

  int len = 20;
  for (unsigned int i = fx.size(); i > (fx.size() - len); --i) {
    int j = -i + 1;
    VLOG(50)
        << "t = " << fx.get_time(j) - t
        << ", fx = " << fx[j]
        << ", dxdt = " << dxdt[j]
        << ", d2xdt2 = " << d2xdt2[j]
        << ", 2x+1 = " << linear[j]
        << ", d(2x+1)/dt = " << dxdt_2[j]
        << ", log = " << log[j];
  }
}

/*
TEST(MovingWindowTest, TupleUsageTest)
{
  using namespace boost;

  typedef tuple<double, double, int> value_t;
  typedef moving_window<value_t, latest<value_t> > time_window;
  typedef std::pair<microsecond_t, value_t> sample;
  typedef std::vector<sample> series;
  typedef series::iterator series_itr;
  typedef series::reverse_iterator series_reverse_itr;

  unsigned int period_duration = 1;
  unsigned int periods = 1000;

  time_window fx(microseconds(period_duration * periods),
                 microseconds(period_duration), value_t(0., 0., 0));

  boost::uint64_t t = now_micros();
  t = t - ( t % period_duration ); // this is so that time lines up nicely.

  series data, expects;
  vector<microsecond_t> times;

  microsecond_t now = now_micros();
  for (unsigned int i = 0; i <= periods*period_duration; ++i) {
    value_t val = value_t(pow(static_cast<double>(i), 2.), 2.*i, 2);
    VLOG(100) << atp::time::to_est(atp::time::as_ptime(t + i))
              <<", i = " << i << ", "
              << '['
              << get<0>(val) << "," << get<1>(val) << "," << get<2>(val)
              << ']';

    microsecond_t now2 = now_micros();
    fx(t + i, val);
    times.push_back(now_micros() - now2);

    data.push_back(sample(t + i, val));
  }
  microsecond_t total_elapsed = now_micros() - now;

  // Calculate expectations based on closing of a period to
  // match the moving_window's sampler (defaults to close)
  microsecond_t period_start = t;
  value_t period_close;
  for (series_itr itr = data.begin(); itr != data.end(); ++itr) {
    if (itr->first - period_start >= period_duration) {
      expects.push_back(sample(period_start, period_close));
      period_start = itr->first;
    }
    // closing value of the period --> always the latest value
    period_close = itr->second;
  }
  // add last current observation
  expects.push_back(sample(period_start, period_close));

  VLOG(50) << "expectations:";
  for (series_itr itr = expects.begin(); itr != expects.end(); ++itr) {
    value_t val = itr->second;
    VLOG(100)
        << atp::time::to_est(atp::time::as_ptime(itr->first)) << ", "
        << "(" << itr->first - t << ", "
        << '['
        << get<0>(val) << "," << get<1>(val) << "," << get<2>(val)
        << ']';
  }

  size_t len = periods + 1;  // history + current observation
  microsecond_t tbuff[len];
  value_t buff[len];
  microsecond_t tbuff2[len];
  value_t buff2[len];
  value_t buff3[len];

  int runs = 10000;
  microsecond_t total1 = 0;
  microsecond_t total2 = 0;
  microsecond_t total3 = 0;
  microsecond_t s = 0;

  for (int i = 0; i < runs; ++i) {
    s = now_micros();
    int copied1 = fx.copy_last(&tbuff[0], &buff[0], len);
    total1 += now_micros() - s;

    EXPECT_EQ(len, copied1);

    s = now_micros();
    int copied2 = fx.copy_last_slow(&tbuff2[0], &buff2[0], len);
    total2 += now_micros() - s;

    EXPECT_EQ(len, copied2);

    s = now_micros();
    int copied3 = fx.copy_last_data(&buff3[0], len);
    total3 += now_micros() - s;

    EXPECT_EQ(len, copied3);
    // EXPECT_EQ(0,
    //           memcmp(static_cast<void*>(&tbuff[0]),
    //                  static_cast<void*>(&tbuff2[0]),
    //                  len * sizeof(microsecond_t)));
    // EXPECT_EQ(0,
    //           memcmp(static_cast<void*>(&buff[0]),
    //                  static_cast<void*>(&buff2[0]),
    //                  len * sizeof(value_t)));
  }
  LOG(INFO) << "copy " << runs << " runs";
  LOG(INFO) << "avg copy time (array_one/two) data only = " << total3/runs;
  LOG(INFO) << "avg copy time (array_one/two)           = " << total1/runs;
  LOG(INFO) << "avg copy time (reverse_itr)             = " << total2/runs;
  LOG(INFO) << "t = " << t;
  LOG(INFO) << "get_time[0] = " << fx.get_time() - t;

  // Compare the sampled data with expectations
  for (unsigned int i = 0; i < len; ++i) {
    value_t val = buff[i];
    VLOG(100)
        << atp::time::to_est(atp::time::as_ptime(tbuff[i])) << ", "
        << "(" << tbuff[i] - t << ", "
        << '['
        << get<0>(val) << "," << get<1>(val) << "," << get<2>(val)
        << "])";

    ASSERT_EQ(expects[i].first, tbuff[i]);
    ASSERT_EQ(get<0>(expects[i].second), get<0>(buff[i]));
    ASSERT_EQ(get<1>(expects[i].second), get<1>(buff[i]));
    ASSERT_EQ(get<2>(expects[i].second), get<2>(buff[i]));

    ASSERT_EQ(get<0>(expects[i].second), get<0>(buff3[i]));
    ASSERT_EQ(get<1>(expects[i].second), get<1>(buff3[i]));
    ASSERT_EQ(get<2>(expects[i].second), get<2>(buff3[i]));
  }
  LOG(INFO) << "Total t = " << total_elapsed << " usec";
  LOG(INFO) << "avg t = " << get_average_time(times);
}

*/

template <typename value_t>
void test_series(unsigned int period_duration,
                 unsigned int periods,
                 unsigned int total_events,
                 function< value_t(int, microsecond_t) > func,
                 const value_t& initial,
                 function< std::string(value_t&) > tostring,
                 function< bool(value_t&, value_t&) > compare,
                 int copy_length, // = periods + 1,
                 int runs = 10000)
{
  using namespace boost;

  typedef moving_window<value_t, latest<value_t> > time_window;
  typedef std::pair<microsecond_t, value_t> sample;
  typedef std::vector<sample> series;
  typedef typename series::iterator series_itr;
  typedef typename series::reverse_iterator series_reverse_itr;

  time_window fx(microseconds(period_duration * periods),
                 microseconds(period_duration), initial);

  boost::uint64_t t = now_micros();
  t = t - ( t % period_duration ); // this is so that time lines up nicely.

  series data, expects, expects_last;
  vector<microsecond_t> times;

  microsecond_t now = now_micros();
  for (unsigned int i = 0; i < total_events; ++i) {
    value_t val = func(i, t + i);
    VLOG(50) << "event: "
             << atp::time::to_est(atp::time::as_ptime(t + i)) << ", "
             << i << ", "
             << t << ", "
             << tostring(val);

    microsecond_t now2 = now_micros();
    fx(t + i, val);
    times.push_back(now_micros() - now2);
    data.push_back(sample(t + i, val));
  }
  microsecond_t total_elapsed = now_micros() - now;

  // Calculate expectations based on closing of a period to
  // match the moving_window's sampler (defaults to close)
  microsecond_t period_start = t;
  value_t period_close;
  for (series_itr itr = data.begin(); itr != data.end(); ++itr) {
    if (itr->first - period_start >= period_duration) {
      expects.push_back(sample(period_start, period_close));
      period_start = itr->first;
    }
    // closing value of the period --> always the latest value
    period_close = itr->second;
  }
  // add last current observation
  expects.push_back(sample(period_start, period_close));

  // Need to truncate the expectations by the last copy_length elements

  VLOG(50) << "expectations:";
  int skip = expects.size() - copy_length;
  for (series_itr itr = expects.begin();
       itr != expects.end();
       ++itr, --skip) {
    if (skip <= 0) expects_last.push_back(*itr);
    value_t val = itr->second;
    VLOG(50)
        << "expect: "
        << atp::time::to_est(atp::time::as_ptime(itr->first)) << ", "
        << itr->first - t << ", "
        << itr->first << ", "
        << tostring(val);
  }

  for (series_itr itr = expects_last.begin();
       itr != expects_last.end();
       ++itr) {
    value_t val = itr->second;
    VLOG(50)
        << "last: "
        << atp::time::to_est(atp::time::as_ptime(itr->first)) << ", "
        << itr->first - t << ", "
        << itr->first << ", "
        << tostring(val);
  }

  microsecond_t tbuff[copy_length];
  value_t buff[copy_length];
  microsecond_t tbuff2[copy_length];
  value_t buff2[copy_length];
  value_t buff3[copy_length];

  microsecond_t total1 = 0;
  microsecond_t total2 = 0;
  microsecond_t total3 = 0;
  microsecond_t s = 0;

  for (int i = 0; i < runs; ++i) {
    s = now_micros();
    int copied1 = fx.copy_last(&tbuff[0], &buff[0], copy_length);
    total1 += now_micros() - s;

    EXPECT_EQ(copy_length, copied1);

    s = now_micros();
    int copied2 = fx.copy_last_slow(&tbuff2[0], &buff2[0], copy_length);
    total2 += now_micros() - s;

    EXPECT_EQ(copy_length, copied2);

    s = now_micros();
    int copied3 = fx.copy_last_data(&buff3[0], copy_length);
    total3 += now_micros() - s;

    EXPECT_EQ(copy_length, copied3);
  }
  LOG(INFO) << "copy " << runs << " runs";
  LOG(INFO) << "avg copy time (array_one/two) data only = " << total3/runs;
  LOG(INFO) << "avg copy time (array_one/two)           = " << total1/runs;
  LOG(INFO) << "avg copy time (reverse_itr)             = " << total2/runs;
  LOG(INFO) << "t = " << t;
  LOG(INFO) << "get_time[0] = " << fx.get_time() - t;

  // Compare the sampled data with expectations
  for (unsigned int i = 1; i <= expects_last.size(); ++i) {
    value_t val = buff[copy_length - i];
    VLOG(50)
        << atp::time::to_est(atp::time::as_ptime(tbuff[i])) << ", "
        << tbuff[copy_length - i] - t << ", "
        << tbuff[copy_length - i] << ", "
        << tostring(val)
        << " ... expects: "
        << expects_last[expects_last.size() - i].first << ", "
        << tostring(expects_last[expects_last.size() - i].second);

    ASSERT_EQ(expects_last[expects_last.size()-i].first, tbuff[copy_length-i]);
    ASSERT_TRUE(compare(buff[copy_length - i], buff3[copy_length - i]));
    ASSERT_TRUE(compare(expects_last[expects_last.size() - i].second,
                        buff[copy_length - i]));
  }
  LOG(INFO) << "Total t = " << total_elapsed << " usec";
  LOG(INFO) << "avg t = " << get_average_time(times);
}

template<typename V>
struct f_2x {
  V operator()(int i, microsecond_t t)
  {
    (void)(t);
    return 2. * i;
  }
};


template<typename V>
struct f_tostring {

  std::string operator()(V& v)
  {
    std::ostringstream os;
    os << v;
    return os.str();
  }
};

template<typename V>
struct f_compare {
  bool operator()(V& expect, V& actual)
  {
    VLOG(50) << "expect = " << expect << ", actual = " << actual;
    return expect == actual;
  }
};

TEST(MovingWindowTest, UsageDoubleTest1)
{
  test_series<double>(1, // period duration
                      5001, // periods
                      1000013, // total events
                      f_2x<double>(),
                      0., // initial
                      f_tostring<double>(),
                      f_compare<double>(),
                      501, // length to copy
                      100000); // runs
}

TEST(MovingWindowTest, UsageDoubleTest2)
{
  test_series<double>(1, // period duration
                      5000, // periods
                      1000013, // total events
                      f_2x<double>(),
                      0., // initial
                      f_tostring<double>(),
                      f_compare<double>(),
                      500, // length to copy
                      100000); // runs
}

TEST(MovingWindowTest, UsageDoubleTest3)
{
  test_series<double>(1, // period duration
                      10, // periods
                      5, // total events
                      f_2x<double>(),
                      0., // initial
                      f_tostring<double>(),
                      f_compare<double>(),
                      4, // length to copy
                      10000); // runs
}

TEST(MovingWindowTest, UsageDoubleTest4)
{
  test_series<double>(1, // period duration
                      10, // periods
                      4, // total events
                      f_2x<double>(),
                      0., // initial
                      f_tostring<double>(),
                      f_compare<double>(),
                      6, // length to copy
                      10000); // runs
}

TEST(MovingWindowTest, UsageDoubleTest5)
{
  test_series<double>(3, // period duration
                      10, // periods
                      4000, // total events
                      f_2x<double>(),
                      0., // initial
                      f_tostring<double>(),
                      f_compare<double>(),
                      5, // length to copy
                      10000); // runs
}


typedef tuple<double, double, int> value_t;
struct f_tuple {
  value_t operator()(int i, microsecond_t t)
  {
    (void)(t);
    return value_t(i * 2., static_cast<double>(i) / 2., i);
  }
};
struct tostring_tuple {
  std::string operator()(value_t& v)
  {
    std::ostringstream os;
    os << "<" << get<0>(v) << "," << get<1>(v) << "," << get<2>(v) << ">";
    return os.str();
  }
};
struct compare_tuple {
  bool operator()(value_t& expect, value_t& actual)
  {
    return get<0>(expect) == get<0>(actual) &&
        get<1>(expect) == get<1>(actual) &&
        get<2>(expect) == get<2>(actual);
  }
};

/*
TEST(MovingWindowTest, TupleTest1)
{
  test_series<value_t>(1, // period duration
                       10000, // periods
                       1000000, // total events
                       f_tuple(),
                       value_t(0., 0., 0), // initial
                       tostring_tuple(),
                       compare_tuple(),
                       1000, // length to copy
                       100000); // runs
}
*/

struct value_struct {
  value_struct(){}
  value_struct(const value_struct& other)
  { first = other.first; second = other.second; third = other.third; }

  value_struct(double first, double second, double third) :
      first(first), second(second), third(third) {}

  double first, second, third;
};
struct f_struct {
  value_struct operator()(int i, microsecond_t t)
  {
    (void)(t);
    return value_struct(i * 2., static_cast<double>(i) / 2., i * 1.);
  }
};
struct tostring_struct {
  std::string operator()(value_struct& v)
  {
    std::ostringstream os;
    os << "<" << v.first << "," << v.second << "," << v.third << ">";
    return os.str();
  }
};
struct compare_struct {
  bool operator()(value_struct& expect, value_struct& actual)
  {
    return expect.first == actual.first &&
        expect.second == actual.second &&
        expect.third == actual.third;
  }
};

/*
TEST(MovingWindowTest, StructTest1)
{
  test_series<value_struct>(1, // period duration
                            10000, // periods
                            1000000, // total events
                            f_struct(),
                            value_struct(0., 0., 0), // initial
                            tostring_struct(),
                            compare_struct(),
                            1000, // length to copy
                            100000); // runs
}
*/
