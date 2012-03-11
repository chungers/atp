
#include <assert.h>
#include <sstream>
#include <map>
#include <vector>

#include <boost/algorithm/string.hpp>
#include "boost/assign.hpp"
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/gregorian/greg_month.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <boost/format.hpp>
#include <boost/scoped_ptr.hpp>


#include <gflags/gflags.h>
#include <glog/logging.h>

#include <leveldb/db.h>
#include <leveldb/iterator.h>

#include "common.hpp"
#include "marketdata_source.hpp"
#include "log_levels.h"
#include "zmq/ZmqUtils.hpp"
#include "proto/historian.hpp"
#include "historian/historian.hpp"


const static std::string NO_VALUE("");

DEFINE_string(leveldb, NO_VALUE, "leveldb file");
DEFINE_bool(est, true, "Use EST for time range.");
DEFINE_string(symbol, NO_VALUE, "symbol");
DEFINE_string(event, NO_VALUE, "event");
DEFINE_string(first, NO_VALUE, "range start");
DEFINE_string(last, NO_VALUE, "range end");


using namespace boost::posix_time;

using namespace ib::internal;
using namespace historian;
using namespace proto::common;
using namespace proto::ib;
using namespace proto::historian;

using boost::optional;
using std::string;

namespace proto {
namespace common {

std::ostream& operator<<(std::ostream& out, const Value& v)
{
  using namespace proto::common;
  switch (v.type()) {
    case Value_Type_INT:
      out << v.int_value();
      break;
    case Value_Type_DOUBLE:
      out << v.double_value();
      break;
    case Value_Type_STRING:
      out << v.string_value();
      break;
  }
  return out;
}

std::ostream& operator<<(std::ostream& out, const MarketData& v)
{
  using namespace historian;
  ptime t =to_est(as_ptime(v.timestamp()));
  out << t << "," << v.symbol() << "," << v.event() << "," << v.value();
  return out;
}

std::ostream& operator<<(std::ostream& out, const MarketDepth& v)
{
  using namespace historian;
  ptime t = to_est(as_ptime(v.timestamp()));
  out << t << "," << v.symbol() << ","
      << v.operation() << "," << v.level() << ","
      << v.side() << "," << v.price() << ","
      << v.size();
  return out;
}

std::ostream& operator<<(std::ostream& out, const IndexedValue& iv)
{
  using namespace historian;
  using namespace proto::common;
  ptime t =to_est(as_ptime(iv.timestamp()));
  out << t << "," << iv.value();
  return out;
}

} // common
} // proto

////////////////////////////////////////////////////////
//
// MAIN
//
int main(int argc, char** argv)
{
  google::SetUsageMessage("Reads and publishes market data from database.");
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  boost::scoped_ptr<historian::Db> db;

  if (FLAGS_leveldb != NO_VALUE) {
    HISTORIAN_LOGGER << "LevelDb on, file = " << FLAGS_leveldb;
    db.reset(new historian::Db(FLAGS_leveldb));
    assert(db->Open());
  }

  boost::posix_time::time_facet facet("%Y-%m-%d %H:%M:%S%F%Q");
  std::cout.imbue(std::locale(std::cout.getloc(), &facet));

  struct Visitor : public historian::Visitor
  {
    virtual bool operator()(const Record& record)
    {
      namespace p = proto::historian;

      if (record.has_key()) {
        std::cout << record.key() << ",";
      }
      switch (record.type()) {

        case SESSION_LOG: {
          optional<SessionLog> log = p::as<SessionLog>(record);
          if (log) {
            std::cout << log->symbol() << ","
                      << "start=" << us_eastern::utc_to_local(
                          historian::as_ptime(log->start_timestamp())) << ","
                      << "end=" << us_eastern::utc_to_local(
                          historian::as_ptime(log->stop_timestamp())) << ","
                      << "source=" << log->source();
          }
        }
          break;
        case INDEXED_VALUE: {
          optional<IndexedValue> value = p::as<IndexedValue>(record);
          if (value) {
            std::cout << *value;
          }
        }
          break;
        case IB_MARKET_DATA: {
          optional<MarketData> value = p::as<MarketData>(record);
          if (value) {
            std::cout << *value;
          }
        }
          break;
        case IB_MARKET_DEPTH: {
          optional<MarketDepth> value = p::as<MarketDepth>(record);
          if (value) {
            std::cout << *value;
          }
        }
          break;
      }

      std::cout << std::endl;
      return true;
    }
  } visit;


  // parse for queries
  if (FLAGS_symbol.size() > 0) {
    // Query by symbol, etc
    QueryBySymbol query;

    query.set_symbol(FLAGS_symbol);

    using namespace proto::historian;
    if (FLAGS_event.size() > 0) {
      query.set_type(INDEXED_VALUE); // index lookup
      query.set_index(FLAGS_event);
    } else {
      query.set_type(IB_MARKET_DATA);
    }

    // compute the actual search keys based on start and end flags
    ptime start;
    ptime end;
    historian::parse(FLAGS_first, &start);
    historian::parse(FLAGS_last, &end);

    query.set_utc_first_micros(historian::as_micros(start));
    query.set_utc_last_micros(historian::as_micros(end));


    int count = db->query(query, &visit);
    std::cout << "Count = " << count;
  } else {
    // Simple by range
    QueryByRange query;

    using namespace proto::historian;
    if (FLAGS_event.size() > 0) {
      query.set_type(INDEXED_VALUE); // index lookup
      query.set_index(FLAGS_event);
    } else {
      query.set_type(IB_MARKET_DATA);
    }

    query.set_first(FLAGS_first);
    query.set_last(FLAGS_last);

    int count = db->query(query, &visit);
    std::cout << "Count = " << count;
  }

  HISTORIAN_LOGGER << "Completed." << std::endl;
  return 0;
}



