
#include <string>
#include <iostream>
#include <vector>

#include <boost/optional.hpp>
#include <gtest/gtest.h>
#include <glog/logging.h>

#include <leveldb/db.h>

#include "proto/common.hpp"
#include "proto/historian.hpp"
#include "historian/historian.hpp"

using std::string;
using boost::optional;
using boost::posix_time::ptime;
using historian::Db;
using proto::common::Value;
using proto::ib::MarketData;
using proto::ib::MarketDepth;
using proto::historian::IndexedValue;
using proto::historian::SessionLog;
using proto::historian::Record;
using proto::historian::Query;
using proto::historian::QueryByRange;
using proto::historian::QueryBySymbol;


TEST(DbTest, DbReadWriteMarketDataTest)
{
  namespace common = proto::common;

  Db db("/tmp/testdb");
  EXPECT_TRUE(db.Open());

  const string est("2012-02-14 04:30:34.567899");
  ptime t;
  atp::time::parse(est, &t);

  MarketData d;

  d.set_timestamp(atp::time::as_micros(t));
  d.set_symbol("AAPL.STK");
  d.set_event("ASK");
  common::set_as(500., d.mutable_value());
  d.set_contract_id(9999);

  EXPECT_TRUE(db.Write(d));

  // if this commits following tests will fail.
  d.mutable_value()->set_double_value(700.);
  EXPECT_FALSE(db.Write(d, false)); // no overwrite

  // now change the key
  d.set_symbol("GOOG.STK");
  EXPECT_TRUE(db.Write(d, true)); // no overwrite - should commit.

  struct : public historian::Visitor
  {
    double value;
    bool fail;
    int count;

    bool operator()(const Record& record)
    {
      return true;
    }

  } visitor;

  visitor.count = 0;
  db.Query("AAPL.STK", "BAC.STK", &visitor);

  EXPECT_EQ(500.0, visitor.value);
  EXPECT_EQ(1, visitor.count);
  EXPECT_FALSE(visitor.fail);

  std::ostringstream key;
  key << "AAPL.STK" << ":" << atp::time::as_micros(t);

  std::ostringstream key2;
  key2 << "AAPL.STK" << ":" << atp::time::as_micros(t) + 1;

  // read it back out
  visitor.count = 0;
  visitor.value = 0.;
  visitor.fail = false;

  db.Query(key.str(), key2.str(), &visitor);

  EXPECT_EQ(500.0, visitor.value);
  EXPECT_EQ(1, visitor.count);
  EXPECT_FALSE(visitor.fail);

  visitor.count = 0;
  visitor.value = 0.;
  visitor.fail = false;
  db.Query(key.str(), key.str(), &visitor);
  EXPECT_EQ(0, visitor.count);

  ///
  {
    visitor.count = 0;
    visitor.value = 0.;
    visitor.fail = false;

    QueryByRange qbr;
    qbr.set_first(key.str());
    qbr.set_last(key2.str());
    qbr.set_index("BID");
    db.Query(qbr, &visitor);
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
    qbr.set_index("ASK");
    db.Query(qbr, &visitor);
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
    qbs.set_utc_first_micros(atp::time::as_micros(t));

    boost::posix_time::time_duration td(0, 0, 0, 1);
    qbs.set_utc_last_micros(atp::time::as_micros(t + td));
    qbs.set_index("ASK");
    db.Query(qbs, &visitor);
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
    qbs.set_utc_first_micros(atp::time::as_micros(t));

    boost::posix_time::time_duration td(0, 0, 0, 1);
    qbs.set_utc_last_micros(atp::time::as_micros(t + td));
    qbs.set_index("ASK");


    db.Query(qbs, &visitor);
    EXPECT_EQ(0, visitor.count);
  }

}

