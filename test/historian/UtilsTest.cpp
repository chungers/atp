
#include <string>
#include <iostream>
#include <vector>

#include <gtest/gtest.h>
#include <glog/logging.h>

#include "proto/common.hpp"
#include "proto/historian.hpp"
#include "historian/historian.hpp"

using boost::uint64_t;
using boost::gregorian::date;
using boost::posix_time::ptime;
using boost::posix_time::time_duration;
using std::string;

typedef boost::date_time::local_adjustor<ptime, -5, us_dst> us_eastern;


TEST(UtilsTest, ValueParsingTest)
{
  using namespace proto::common;

  Value v;
  v.set_type(proto::common::Value_Type_INT);
  v.set_int_value(20);

  boost::optional<int> intValue = as<int>(v);
  EXPECT_TRUE(intValue);
  EXPECT_EQ(20, *intValue);

  // We don't expect a value as double since the proto was
  // encoded as an int value.
  boost::optional<double> doubleValue = as<double>(v);
  EXPECT_FALSE(doubleValue);

  // Test wrapping in a Value
  Value v2 = wrap(20);
  boost::optional<int> intValue2 = as<int>(v2);
  EXPECT_EQ(20, *intValue2);

  v2 = wrap(20.0);
  intValue2 = as<int>(v2);
  EXPECT_FALSE(intValue2);

  v2 = wrap(string("foo"));
  intValue2 = as<int>(v2);
  EXPECT_FALSE(intValue2);

  boost::optional<string> stringVal = as<string>(v2);
  EXPECT_TRUE(stringVal);
  EXPECT_EQ("foo", *stringVal);


  v2 = wrap("bar");
  intValue2 = as<int>(v2);
  EXPECT_FALSE(intValue2);

  stringVal = as<string>(v2);
  EXPECT_TRUE(stringVal);
  EXPECT_EQ("bar", *stringVal);


  Value v3;
  set_as<int>(25, &v3);
  optional<int> intValue3 = as<int>(v3);
  EXPECT_TRUE(intValue3);
  EXPECT_EQ(25, *intValue3);
}

TEST(UtilsTest, RecordParsingTest)
{
  namespace common = proto::common;
  namespace ib = proto::ib;
  namespace historian = proto::historian;

  common::Value v = common::wrap<string>("a test");

  historian::Record r = historian::wrap<common::Value>(v);

  boost::optional<ib::MarketData> om = historian::as<ib::MarketData>(r);
  EXPECT_FALSE(om);

  boost::optional<common::Value> ov = historian::as<common::Value>(r);
  EXPECT_TRUE(ov);
  EXPECT_EQ("a test", *common::as<string>(*ov));

  ib::MarketData md;
  md.set_event("ASK");
  md.set_symbol("AAPL");
  md.set_contract_id(1234);
  md.mutable_value()->CopyFrom(common::wrap<double>(450.));

  r = historian::wrap<ib::MarketData>(md);
  boost::optional<ib::MarketDepth> dp = historian::as<ib::MarketDepth>(r);
  EXPECT_FALSE(dp);
  boost::optional<ib::MarketData> od = historian::as<ib::MarketData>(r);
  EXPECT_TRUE(od);

  historian::set_as<ib::MarketData>(md, &r);
  boost::optional<ib::MarketData> od2 = historian::as<ib::MarketData>(r);
  EXPECT_TRUE(od2);

}


TEST(UtilsTest, DateTest)
{
  ptime t1, t2;

  historian::parse("2012-02-14 04:30:34.567899", &t1, false);
  historian::parse("2012-02-14 04:36:34.567899", &t2, false);

  time_duration diff = t2 - t1;
  EXPECT_EQ(6, diff.minutes());
}

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
