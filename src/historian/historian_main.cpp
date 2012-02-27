
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
#include "historian/historian.hpp"

const static std::string NO_VALUE("__no_value__");

DEFINE_bool(rth, true, "Regular trading hours");
DEFINE_bool(delay, false, "True to simulate sample delays when publishing");
DEFINE_string(endpoint, "tcp://127.0.0.1:5555", "Endpoint for publishing.");
DEFINE_bool(publish, false, "True to publish to endpoint.");
DEFINE_int64(playback, 1, "N multiple speed in playback.");
DEFINE_string(leveldb, NO_VALUE, "leveldb file");
DEFINE_bool(csv, true, "True to write to stdout (when not publishing)");

DEFINE_bool(est, true, "Use EST for time range.");
DEFINE_string(symbol, NO_VALUE, "symbol");
DEFINE_string(start, NO_VALUE, "range start");
DEFINE_string(end, NO_VALUE, "range end");


using namespace ib::internal;

typedef std::pair<std::string, std::string> range;
static const range parseRange()
{
  namespace bt = boost::posix_time;

  if (FLAGS_symbol != NO_VALUE) {
    // compute the actual search keys based on start and end flags
    bt::ptime start;
    bt::ptime end;
    historian::parse(FLAGS_start, &start);
    historian::parse(FLAGS_end, &end);

    std::ostringstream sStart;
    std::ostringstream sEnd;
    sStart << FLAGS_symbol << ":" << historian::as_micros(start);
    sEnd << FLAGS_symbol << ":" << historian::as_micros(end);
    return range(sStart.str(), sEnd.str());
  } else {
    return range(FLAGS_start, FLAGS_end);
  }
}


////////////////////////////////////////////////////////
//
// MAIN
//
int main(int argc, char** argv)
{
  google::SetUsageMessage("Reads and publishes market data from database.");
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
  boost::int64_t last_ts = 0;
  const range& range = parseRange();

  LOG(INFO) << "Using range [" << range.first << ", " << range.second << ")";

  // The lines are space separated, so we need to skip whitespaces.
  for (iterator->Seek(range.first);
       iterator->Valid() && iterator->key().ToString() < range.second;
       iterator->Next()) {

    leveldb::Slice key = iterator->key();
    leveldb::Slice value = iterator->value();

    using namespace proto::historian;

    Record record;

    boost::int64_t dt;
    bool rth;
    boost::posix_time::ptime t;

    if (record.ParseFromString(value.ToString())) {
      if (record.type() == Record_Type_SESSION_LOG) {
        const proto::historian::SessionLog& event = record.session_log();
        boost::posix_time::ptime start =
            historian::as_ptime(event.start_timestamp());
        boost::posix_time::ptime end =
            historian::as_ptime(event.stop_timestamp());
        const std::string& source = event.source();
        std::cout << key.ToString() << ","
                  << event.symbol() << ","
                  << "start=" << us_eastern::utc_to_local(start) << ","
                  << "end=" << us_eastern::utc_to_local(end) << ","
                  << "source=" << source << std::endl;

      } else if (record.type() == Record_Type_IB_MARKET_DATA) {
          const proto::ib::MarketData& event = record.ib_marketdata();
          t = historian::as_ptime(event.timestamp());
          rth = historian::checkRTH(t);

          if (!rth && FLAGS_rth) {
            // Skip if not RTH and we want only data during trading hours.
            continue;
          }

          if (FLAGS_publish) {

            boost::int64_t dt = event.timestamp() - last_ts;
            if (last_ts > 0 && dt > 0 && FLAGS_delay) {
              // wait dt micros
              usleep(dt / FLAGS_playback);
            }
            atp::MarketData<double> marketData(event.symbol(),
                                               event.timestamp(),
                                               event.event(),
                                               event.double_value());
            size_t sent = marketData.dispatch(socket);
            //std::cerr << event.symbol << " " << sent << std::endl;
            last_ts = event.timestamp();

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
            std::cout << "," << key.ToString();
            std::cout << std::endl;
          }
      } else if (record.type() == Record_Type_IB_MARKET_DATA) {
          const proto::ib::MarketDepth& depth = record.ib_marketdepth();
          t = historian::as_ptime(depth.timestamp());
          rth = historian::checkRTH(t);

          if (!rth && FLAGS_rth) {
            // Skip if not RTH
            continue;
          }

          if (FLAGS_publish) {

            boost::int64_t dt = depth.timestamp() - last_ts;
            if (last_ts > 0 && dt > 0 && FLAGS_delay) {
              // wait dt micros
              usleep(dt / FLAGS_playback);
            }

            atp::MarketDepth marketDepth(depth.symbol(),
                                         depth.timestamp(),
                                         depth.side(),
                                         depth.level(),
                                         depth.operation(),
                                         depth.price(),
                                         depth.size(),
                                         depth.mm());
            size_t sent = marketDepth.dispatch(socket);
            last_ts = depth.timestamp();
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
    // Need to delete this in order.  The better way is to
    // have a class where the member variables are scoped_ptr
    // and have the destructor do the proper ordering of things.
    iterator.reset();
    delete levelDb;
  }
  HISTORIAN_LOGGER << "Completed." << std::endl;
  return 0;
}



