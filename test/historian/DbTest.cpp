
#include <string>
#include <iostream>
#include <vector>

#include <gtest/gtest.h>
#include <glog/logging.h>


#include "historian/historian.hpp"

using boost::posix_time::ptime;
using historian::Db;
using proto::ib::MarketData;
using proto::ib::MarketDepth;
using proto::historian::SessionLog;
using proto::historian::Query;
using proto::historian::QueryByRange;
using proto::historian::QueryBySymbol;



TEST(DbTest, DbReadWriteMarketDataTest)
{
  Db db("/tmp/testdb");
  EXPECT_TRUE(db.open());

  const string est("2012-02-14 04:30:34.567899");
  ptime t;
  historian::parse(est, &t);

  MarketData d;

  d.set_timestamp(historian::as_micros(t));
  d.set_symbol("AAPL.STK");
  d.set_event("ASK");
  d.set_type(proto::ib::MarketData_Type_DOUBLE);
  d.set_double_value(500.0);
  d.set_contract_id(9999);

  EXPECT_TRUE(db.write(d));

  d.set_double_value(700.);  // if this commits following tests will fail.
  EXPECT_FALSE(db.write(d, false)); // no overwrite

  // now change the key
  d.set_symbol("GOOG.STK");
  EXPECT_TRUE(db.write(d, true)); // no overwrite - should commit.

  struct : public historian::Visitor
  {
    double value;
    bool fail;
    int count;

    bool operator()(const SessionLog& log)
    {
      fail = true;
      return false;
    }

    bool operator()(const MarketData& data)
    {
      value = data.double_value();
      count++;
      fail = false;
      return true;
    }

    bool operator()(const MarketDepth& data)
    {
      fail = true;
      return false;
    }

  } visitor;

  visitor.count = 0;
  db.query("AAPL.STK", "BAC.STK", &visitor);

  EXPECT_EQ(500.0, visitor.value);
  EXPECT_EQ(1, visitor.count);
  EXPECT_FALSE(visitor.fail);

  std::ostringstream key;
  key << "AAPL.STK" << ":" << historian::as_micros(t);

  std::ostringstream key2;
  key2 << "AAPL.STK" << ":" << historian::as_micros(t) + 1;

  // read it back out
  visitor.count = 0;
  visitor.value = 0.;
  visitor.fail = false;

  db.query(key.str(), key2.str(), &visitor);

  EXPECT_EQ(500.0, visitor.value);
  EXPECT_EQ(1, visitor.count);
  EXPECT_FALSE(visitor.fail);

  visitor.count = 0;
  visitor.value = 0.;
  visitor.fail = false;
  db.query(key.str(), key.str(), &visitor);
  EXPECT_EQ(0, visitor.count);

  ///
  {
    visitor.count = 0;
    visitor.value = 0.;
    visitor.fail = false;

    QueryByRange qbr;
    qbr.set_first(key.str());
    qbr.set_last(key2.str());
    qbr.set_filter("BID");
    db.query(qbr, &visitor);
    EXPECT_EQ(0, visitor.count);
  }

  ///
  {
    visitor.count = 0;
    visitor.value = 0.;
    visitor.fail = false;

    QueryByRange qbr;
    qbr.set_first(key.str());
    qbr.set_last(key2.str());
    qbr.set_filter("ASK");
    db.query(qbr, &visitor);
    EXPECT_EQ(1, visitor.count);
    EXPECT_EQ(500.0, visitor.value);
  }

  ///
  {
    visitor.count = 0;
    visitor.value = 0.;
    visitor.fail = false;

    QueryBySymbol qbs;
    qbs.set_symbol("AAPL.STK");
    qbs.set_utc_first_micros(historian::as_micros(t));

    boost::posix_time::time_duration td(0, 0, 0, 1);
    qbs.set_utc_last_micros(historian::as_micros(t + td));
    qbs.set_filter("ASK");
    db.query(qbs, &visitor);
    EXPECT_EQ(1, visitor.count);
    EXPECT_EQ(500.0, visitor.value);
  }

  ///
  {
    visitor.count = 0;
    visitor.value = 0.;
    visitor.fail = false;

    QueryBySymbol qbs;
    qbs.set_symbol("AAPL.STK");
    qbs.set_utc_first_micros(historian::as_micros(t));

    boost::posix_time::time_duration td(0, 0, 0, 1);
    qbs.set_utc_last_micros(historian::as_micros(t + td));
    qbs.set_filter("ASK");

    // only market depth
    qbs.set_depth_only(true);

    db.query(qbs, &visitor);
    EXPECT_EQ(0, visitor.count);
  }

}

