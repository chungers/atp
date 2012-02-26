
#include <string>
#include <iostream>
#include <vector>

#include <gtest/gtest.h>
#include <glog/logging.h>


#include "historian/historian.hpp"

using boost::uint64_t;
using boost::gregorian::date;
using boost::posix_time::ptime;
using boost::posix_time::time_duration;
using std::string;

typedef boost::date_time::local_adjustor<ptime, -5, us_dst> us_eastern;

TEST(UtilsTest, ParseTest)
{
  ptime parsed;
  const string utc("2012-02-14 04:30:34.567899");

  EXPECT_TRUE(historian::parse(utc, &parsed, false));
  date d = parsed.date();
  EXPECT_EQ(2012, d.year());
  EXPECT_EQ(2, d.month());
  EXPECT_EQ(14, d.day());

  time_duration td = parsed.time_of_day();
  EXPECT_EQ(4, td.hours());
  EXPECT_EQ(30, td.minutes());
  EXPECT_EQ(34, td.seconds());


  // parsed is always in utc
  const string est("2012-02-14 04:30:34.567899");

  EXPECT_TRUE(historian::parse(est, &parsed));
  d = parsed.date();
  EXPECT_EQ(2012, d.year());
  EXPECT_EQ(2, d.month());
  EXPECT_EQ(14, d.day());

  td = us_eastern::utc_to_local(parsed).time_of_day();

  EXPECT_EQ(4, td.hours());
  EXPECT_EQ(30, td.minutes());
  EXPECT_EQ(34, td.seconds());

}

TEST(UtisTest, MicrosTimeTest)
{
  ptime utc, utc2;
  const string est("2012-02-14 04:30:34.567899");
  const string est2("2012-02-14 04:30:34");

  historian::parse(est, &utc);
  historian::parse(est2, &utc2);

  EXPECT_GT(utc.time_of_day().total_microseconds(),
            utc2.time_of_day().total_microseconds());

  ptime fromMicros = historian::as_ptime(historian::as_micros(utc));
  EXPECT_EQ(utc, fromMicros);

  ptime fromMicros2 = historian::as_ptime(historian::as_micros(utc2));
  EXPECT_EQ(utc2, fromMicros2);
}
