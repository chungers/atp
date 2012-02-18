
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
#include "proto/ib.pb.h"
#include "proto/historian.pb.h"
#include "zmq/ZmqUtils.hpp"


const static std::string NO_VALUE("__no_value__");

DEFINE_bool(rth, true, "Regular trading hours");
DEFINE_bool(delay, true, "True to simulate sample delays when publishing");
DEFINE_string(endpoint, "tcp://127.0.0.1:5555", "Endpoint for publishing.");
DEFINE_bool(publish, true, "True to publish to endpoint.");
DEFINE_int64(playback, 1, "N multiple speed in playback.");
DEFINE_string(leveldb, NO_VALUE, "leveldb file");
DEFINE_bool(csv, true, "True to write to stdout (when not publishing)");

DEFINE_string(start, NO_VALUE, "range start");
DEFINE_string(end, NO_VALUE, "range end");

namespace atp {
namespace utils {


static std::map<std::string, std::string> ACTIONS = boost::assign::map_list_of
    ("reqMktData", "contract")
    ("reqMktDepth", "contract")
    ("reqRealTimeBars", "contract")
    ("reqHistoricalData", "contract")
    ;

typedef std::pair<std::string, proto::ib::MarketData_Type> event_type;
static std::map<std::string, event_type> EVENTS =
    boost::assign::map_list_of
    ("tickPrice", event_type("price", proto::ib::MarketData_Type_DOUBLE))
    ("tickSize", event_type("size", proto::ib::MarketData_Type_INT))
    ("tickGeneric", event_type("value", proto::ib::MarketData_Type_STRING))
    ;

static std::map<long,std::string> TickerIdSymbolMap;

// In America/New_York
static boost::posix_time::time_duration RTH_START(9, 30, 0, 0);
static boost::posix_time::time_duration RTH_END(16, 0, 0, 0);

using namespace boost::posix_time;

static ptime getPosixTime(boost::uint64_t ts)
{
  ptime t = from_time_t(ts / 1000000LL);
  time_duration micros(0, 0, 0, ts % 1000000LL);
  t += micros;
  return t;
}

static bool checkRTH(ptime t)
{
  time_duration eastern = us_eastern::utc_to_local(t).time_of_day();
  return eastern >= RTH_START && eastern < RTH_END;
}

} // namespace utils
} // namespace atp

using namespace ib::internal;


////////////////////////////////////////////////////////
//
// MAIN
//
int main(int argc, char** argv)
{
  google::SetUsageMessage("Reads and publishes market data from logfile.");
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  leveldb::DB* levelDb = NULL;
  leveldb::Options options;

  if (FLAGS_leveldb != NO_VALUE) {
    HISTORIAN_LOGGER << "LevelDb on, file = " << FLAGS_leveldb;
    options.create_if_missing = true;
    leveldb::Status status = leveldb::DB::Open(
        options, FLAGS_leveldb, &levelDb);
    assert(status.ok());
  }

  boost::posix_time::time_facet facet("%Y-%m-%d %H:%M:%S%F%Q");
  std::cout.imbue(std::locale(std::cout.getloc(), &facet));

  zmq::context_t* context = NULL;
  zmq::socket_t* socket = NULL;

  if (FLAGS_csv) {
    std::cout << "\"timestamp_utc\",\"symbol\",\"event\",\"value\""
              << std::endl;
  }

  if (FLAGS_publish) {
    context = new zmq::context_t(1);
    socket = new zmq::socket_t(*context, ZMQ_PUSH);
    socket->connect(FLAGS_endpoint.c_str());

    LOG(INFO) << "Pushing events to " << FLAGS_endpoint;
  }

  boost::scoped_ptr<leveldb::Iterator> iterator(
      levelDb->NewIterator(leveldb::ReadOptions()));

  using namespace boost::posix_time;
  ptime epoch(boost::gregorian::date(1970,boost::gregorian::Jan,1));

  boost::int64_t last_ts = 0;

  // The lines are space separated, so we need to skip whitespaces.
  for (iterator->Seek(FLAGS_start);
       iterator->Valid() && iterator->key().ToString() < FLAGS_end;
       iterator->Next()) {

    leveldb::Slice key = iterator->key();
    leveldb::Slice value = iterator->value();

    using namespace proto::historian;

    Record record;

    boost::int64_t dt;
    bool rth;
    boost::posix_time::ptime t;

    if (record.ParseFromString(value.ToString())) {
      if (record.type() == Record_Type_IB_MARKET_DATA) {
          const proto::ib::MarketData& event = record.ib_marketdata();
          t = atp::utils::getPosixTime(event.time_stamp());
          rth = atp::utils::checkRTH(t);

          if (!rth && FLAGS_rth) {
            // Skip if not RTH and we want only data during trading hours.
            continue;
          }

          if (FLAGS_publish) {

            boost::int64_t dt = event.time_stamp() - last_ts;
            if (last_ts > 0 && dt > 0 && FLAGS_delay) {
              // wait dt micros
              usleep(dt / FLAGS_playback);
            }
            atp::MarketData<double> marketData(event.symbol(),
                                               event.time_stamp(),
                                               event.event(),
                                               event.double_value());
            size_t sent = marketData.dispatch(socket);
            //std::cerr << event.symbol << " " << sent << std::endl;
            last_ts = event.time_stamp();

          }

          if (FLAGS_csv) {
            std::cout << t << ","
                      << event.symbol() << ","
                      << event.event() << ",";
            switch (event.type()) {
              case (proto::ib::MarketData_Type_INT) :
                std::cout << event.int_value();
                break;
              case (proto::ib::MarketData_Type_DOUBLE) :
                std::cout << event.double_value();
                break;
              case (proto::ib::MarketData_Type_STRING) :
                std::cout << event.string_value();
                break;
            }
            std::cout << std::endl;
          }
      } else if (record.type() == Record_Type_IB_MARKET_DATA) {
          const proto::ib::MarketDepth& depth = record.ib_marketdepth();
          t = atp::utils::getPosixTime(depth.time_stamp());
          rth = atp::utils::checkRTH(t);

          if (!rth && FLAGS_rth) {
            // Skip if not RTH
            continue;
          }

          if (FLAGS_publish) {

            boost::int64_t dt = depth.time_stamp() - last_ts;
            if (last_ts > 0 && dt > 0 && FLAGS_delay) {
              // wait dt micros
              usleep(dt / FLAGS_playback);
            }

            atp::MarketDepth marketDepth(depth.symbol(),
                                         depth.time_stamp(),
                                         depth.side(),
                                         depth.level(),
                                         depth.operation(),
                                         depth.price(),
                                         depth.size(),
                                         depth.mm());
            size_t sent = marketDepth.dispatch(socket);
            last_ts = depth.time_stamp();
          }

          if (FLAGS_csv) {
            std::cout << t << ","
                      << depth.symbol() << ","
                      << depth.side() << ","
                      << depth.level() << ","
                      << depth.operation() << ","
                      << depth.price() << ","
                      << depth.size() << ","
                      << depth.mm() << std::endl;
          }
      }
    }
  }

  if (FLAGS_publish) {
    delete socket;
    delete context;
  }

  if (levelDb != NULL) {
    delete levelDb;
  }
  HISTORIAN_LOGGER << "Completed." << std::endl;
  return 0;
}



