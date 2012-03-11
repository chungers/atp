
#include <assert.h>
#include <iostream>
#include <fstream>
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
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/shared_ptr.hpp>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include <leveldb/db.h>

#include "log_levels.h"
#include "ib/ticker_id.hpp"
#include "ib/tick_types.hpp"
#include "ib/EventDispatcherBase.hpp"
#include "ib/TickerMap.hpp"
#include "historian/historian.hpp"
#include "zmq/ZmqUtils.hpp"

const static std::string NO_VALUE("__no_value__");

DEFINE_bool(rth, false, "Regular trading hours");
DEFINE_bool(delay, false, "True to simulate sample delays when publishing");
DEFINE_string(logfile, "logfile", "The name of the log file.");
DEFINE_string(endpoint, "tcp://127.0.0.1:5555", "Endpoint for publishing.");
DEFINE_bool(publish, false, "True to publish to endpoint.");
DEFINE_int64(playback, 1, "N multiple speed in playback.");
DEFINE_string(leveldb, NO_VALUE, "leveldb file");
DEFINE_bool(csv, false, "True to write to stdout (when not publishing)");
DEFINE_int64(gapInMinutes, 5,
             "Number of minutes gap to start a new session log");
DEFINE_bool(checkDuplicate, false, "True to read for existing record before writing db.");


using namespace boost::posix_time;
using namespace historian;

namespace atp {
namespace utils {


static std::map<std::string, std::string> ACTIONS = boost::assign::map_list_of
    ("reqMktData", "contract")
    ("reqMktDepth", "contract")
    ("reqRealTimeBars", "contract")
    ("reqHistoricalData", "contract")
    ;

typedef std::pair<std::string, proto::common::Value_Type> event_type;
static std::map<std::string, event_type> EVENTS =
    boost::assign::map_list_of
    ("tickPrice", event_type("price", proto::common::Value_Type_DOUBLE))
    ("tickSize", event_type("size", proto::common::Value_Type_INT))
    ("tickGeneric", event_type("value", proto::common::Value_Type_STRING))
    ;

static std::map<long,std::string> TickerIdSymbolMap;
typedef boost::shared_ptr<SessionLog> SessionLogPtr;
typedef std::map<std::string, SessionLogPtr> SessionLogMap;
typedef std::map<std::string, SessionLogPtr>::const_iterator SessionLogMapIterator;
static SessionLogMap SymbolSessionLogs;

static void register_ticker_id_symbol(const long tickerId,
                                      const std::string& symbol)
{
  TickerIdSymbolMap[tickerId] = symbol;
}

static void GetSymbol(long code, std::string* symbol)
{
  if (TickerIdSymbolMap.find(code) == TickerIdSymbolMap.end()) {
    LOG(WARNING) << "No mapping exists for ticker id " << code;
    std::string sym;
    ib::internal::SymbolFromTickerId(code, &sym);
    register_ticker_id_symbol(code, sym + ".STK");
    LOG(INFO) << "Mapping " << code << " to " << TickerIdSymbolMap[code];
  }
  *symbol = TickerIdSymbolMap[code];
}

static bool checkEvent(std::map<std::string, std::string>& event)
{
  std::map<std::string, std::string>::iterator found = event.find("event");
  if (found != event.end()) {
    return EVENTS.find(event["event"]) != EVENTS.end();
  }
  return false;
}

static bool checkBookEvent(std::map<std::string, std::string>& event)
{
  std::map<std::string, std::string>::iterator found = event.find("event");
  if (found != event.end()) {
    return found->second == "updateMktDepth" ||
        found->second == "updateMktDepthL2";
  }
  return false;
}

static bool checkAction(std::map<std::string, std::string>& event)
{
  std::map<std::string, std::string>::iterator found = event.find("action");
  if (found != event.end()) {
    return ACTIONS.find(event["action"]) != ACTIONS.end();
  }
  return false;
}

static bool GetField(std::map<std::string, std::string>& nv,
                     std::string key, uint64_t* out)
{
  std::map<std::string, std::string>::iterator found = nv.find(key);
  if (found != nv.end()) {
    char* end;
    *out = static_cast<uint64_t>(strtoll(found->second.c_str(), &end, 10));
    return true;
  }
  return false;
}

static bool GetField(std::map<std::string, std::string>& nv,
                     std::string key, std::string* out)
{
  std::map<std::string, std::string>::iterator found = nv.find(key);
  if (found != nv.end()) {
    out->assign(nv[key]);
    return true;
  }
  return false;
}

static bool GetField(std::map<std::string, std::string>& nv,
                     std::string key, int* out)
{
  std::map<std::string, std::string>::iterator found = nv.find(key);
  if (found != nv.end()) {
    *out = atoi(nv[key].c_str());
    return true;
  }
  return false;
}

static bool GetField(std::map<std::string, std::string>& nv,
                     std::string key, long* out)
{
  std::map<std::string, std::string>::iterator found = nv.find(key);
  if (found != nv.end()) {
    *out = atol(nv[key].c_str());
    return true;
  }
  return false;
}

static bool GetField(std::map<std::string, std::string>& nv,
                     std::string key, double* out)
{
  std::map<std::string, std::string>::iterator found = nv.find(key);
  if (found != nv.end()) {
    *out = atof(nv[key].c_str());
    return true;
  }
  return false;
}

inline static bool ParseMap(const std::string& line,
                            std::map<std::string, std::string>& result,
                            const char nvDelim,
                            const std::string& pDelim) {
  VLOG(20) << "Parsing " << line;

  // Split by the '=' into name-value pairs:
  vector<std::string> nvpairs_vec;
  boost::split(nvpairs_vec, line, boost::is_any_of(pDelim));

  if (nvpairs_vec.empty()) {
    return false;
  }

  // Split the name-value pairs by '=' or nvDelim into a map:
  vector<std::string>::iterator itr;
  for (itr = nvpairs_vec.begin(); itr != nvpairs_vec.end(); ++itr) {
    int sep = itr->find(nvDelim);
    std::string key = itr->substr(0, sep);
    std::string val = itr->substr(sep + 1, itr->length() - sep);
    LOG_READER_DEBUG << "Name-Value Pair: " << *itr
                     << " (" << key << ", " << val << ") " << endl;
    result[key] = val;
  }
  LOG_READER_DEBUG << "Map, size=" << result.size() << endl;
  return true;
}


static bool MapActions(std::map<std::string, std::string>& nv)
{
  // Get id and contract and store the mapping.
  long tickerId = -1;
  if (!GetField(nv, "id", &tickerId)) {
    return false;
  }
  std::string symbol;
  std::string contractString;
  if (!GetField(nv, "contract", &contractString)) {
    // Use the computed symbol/id rule:
    GetSymbol(tickerId, &symbol);

  } else {
    // Build a symbol string from the contract spec.
    // This is a ; separate list of name:value pairs.
    std::map<std::string, std::string> nv; // basic name /value pair
    if (ParseMap(contractString, nv, ':', ";")) {

      bool ok = ib::internal::symbol_from_contract(nv, &symbol);
      LOG(INFO) << "Using symbol from contract " << symbol;
      if (!ok) {
        LOG(FATAL) << "Failed to generate symbol from parsed contract spec: "
                   << contractString;
      }
      if (TickerIdSymbolMap.find(tickerId) == TickerIdSymbolMap.end()) {
        register_ticker_id_symbol(tickerId, symbol);

        LOG(INFO) << "Created mapping of " << tickerId << " to " << symbol;
      } else {
        // We may have a collision!
        if (TickerIdSymbolMap[tickerId] != symbol) {
          LOG(FATAL) << "collision for " << tickerId << ": "
                     << symbol << " and " << TickerIdSymbolMap[tickerId];
          return false;
        }
      }

    } else {
      LOG(FATAL) << "Failed to parse contract spec: " << contractString;
    }
  }

  return true;
}

static bool Convert(std::map<std::string, std::string>& nv,
                    historian::MarketData* result)
{
  using std::string;

  uint64_t ts = 0;
  ////////// Timestamp
  if (!GetField(nv, "ts_utc", &ts)) {
    return false;
  }
  result->set_timestamp(ts);
  LOG_READER_DEBUG << "Timestamp ==> " << result->timestamp() << endl;

  ////////// TickerId / Symbol:
  long code = 0;
  if (!GetField(nv, "tickerId", &code)) {
    return false;
  }
  std::string symbol;
  GetSymbol(code, &symbol);

  result->set_contract_id(code);

  nv["symbol"] = symbol;
  result->set_symbol(symbol);
  LOG_READER_DEBUG << "symbol ==> " << result->symbol()
                   << "(" << code << ")" << endl;

  ////////// Event
  std::string event;
  if (!GetField(nv, "field", &event)) {
    return false;
  }
  result->set_event(event);
  LOG_READER_DEBUG << "event ==> " << result->event() << endl;

  ////////// Price / Size
  using proto::common::Value_Type;

  std::string method;
  double value;
  if (GetField(nv, "event", &method)) {
    event_type ft = EVENTS[method];
    std::string fieldToUse = ft.first;
    Value_Type type = ft.second;

    if (!GetField(nv, fieldToUse, &value)) {
      return false;
    }
    result->mutable_value()->set_type(type);
    switch (type) {
      case proto::common::Value_Type_INT :
        result->mutable_value()->set_int_value(value);
        break;
      case proto::common::Value_Type_DOUBLE :
        result->mutable_value()->set_double_value(value);
        break;
      case proto::common::Value_Type_STRING :
        result->mutable_value()->set_string_value(""); // TODO
        break;
    }
  }
  LOG_READER_DEBUG << "value ==> " << result->value().double_value() << endl;

  return true;
}

static bool Convert(std::map<std::string, std::string>& nv,
                    historian::MarketDepth* result)
{
  using std::string;

  ////////// Timestamp
  boost::uint64_t ts;
  if (!GetField(nv, "ts_utc", &ts)) {
    return false;
  }
  result->set_timestamp(ts);
  LOG_READER_DEBUG << "Timestamp ==> " << result->timestamp();

  ////////// TickerId / Symbol:
  long code = 0;
  if (!GetField(nv, "id", &code)) {
    return false;
  }
  result->set_contract_id(code);

  string parsed_symbol;
  GetSymbol(code, &parsed_symbol);
  nv["symbol"] = parsed_symbol;
  result->set_symbol(parsed_symbol);
  LOG_READER_DEBUG << "symbol ==> " << result->symbol()
                   << "(" << code << ")";

  ////////// Side
  int parsed_side;
  if (!GetField(nv, "side", &parsed_side)) {
    return false;
  }
  switch (parsed_side) {
    case (proto::ib::MarketDepth_Side_ASK) :
      result->set_side(proto::ib::MarketDepth_Side_ASK);
      break;
    case (proto::ib::MarketDepth_Side_BID) :
      result->set_side(proto::ib::MarketDepth_Side_BID);
      break;
  }
  LOG_READER_DEBUG << "side ==> " << result->side();

  ////////// Operation
  int parsed_operation;
  if (!GetField(nv, "operation", &parsed_operation)) {
    return false;
  }
  switch (parsed_operation) {
    case (proto::ib::MarketDepth_Operation_INSERT) :
      result->set_operation(proto::ib::MarketDepth_Operation_INSERT);
      break;
    case (proto::ib::MarketDepth_Operation_DELETE) :
      result->set_operation(proto::ib::MarketDepth_Operation_DELETE);
      break;
    case (proto::ib::MarketDepth_Operation_UPDATE) :
      result->set_operation(proto::ib::MarketDepth_Operation_UPDATE);
      break;
  }
  LOG_READER_DEBUG << "operation ==> " << result->operation();

  ////////// Level
  int parsed_level;
  if (!GetField(nv, "position", &parsed_level)) {
    return false;
  }
  result->set_level(parsed_level);
  LOG_READER_DEBUG << "level ==> " << result->level();

  ////////// Price
  double parsed_price;
  if (!GetField(nv, "price", &parsed_price)) {
    return false;
  }
  result->set_price(parsed_price);
  LOG_READER_DEBUG << "price ==> " << result->price();

  ////////// Size
  int parsed_size;
  if (!GetField(nv, "size", &parsed_size)) {
    return false;
  }
  result->set_size(parsed_size);
  LOG_READER_DEBUG << "size ==> " << result->size();

  ////////// MM (optional)
  string parsed_mm;
  if (!GetField(nv, "marketMaker", &parsed_mm)) {
    result->set_mm("L1");
  }
  result->set_mm(parsed_mm);
  LOG_READER_DEBUG << "mm ==> " << result->mm();
  return true;
}

template <typename T>
static void updateSessionLog(const T& data,
                             const boost::shared_ptr<historian::Db>& db)
{
  using proto::historian::SessionLog;
  using boost::posix_time::ptime;
  using boost::posix_time::time_duration;

  const std::string& symbol = data.symbol();
  if (SymbolSessionLogs.find(symbol) == SymbolSessionLogs.end()) {

    SymbolSessionLogs[symbol] = SessionLogPtr(new SessionLog());
    SymbolSessionLogs[symbol]->set_symbol(symbol);
    SymbolSessionLogs[symbol]->set_start_timestamp(data.timestamp());
    SymbolSessionLogs[symbol]->set_stop_timestamp(0);

  } else {

    const SessionLogPtr& log = SymbolSessionLogs[symbol];
    // We check to see if more than X minutes have elapsed.  If so,
    // write one session log and start a new one.
    if (log->stop_timestamp() > 0) {

      ptime last = historian::as_ptime(log->stop_timestamp());
      ptime now = historian::as_ptime(data.timestamp());

      time_duration diff = now - last;
      if (diff.minutes() > FLAGS_gapInMinutes) {
        LOG(INFO) << "Starting a new session log range for " << symbol
                  << " with gap " << diff << " between "
                  << last << " and " << now;

        const std::string filename = FLAGS_logfile;
        std::vector<std::string> parts;
        boost::split(parts, filename, boost::is_any_of("/"));
        const std::string& source = parts.back();
        log->set_source(source);

        if (!db->Write(*log) > 0) {
          LOG(ERROR) << "Failed to write session log";
        }

        // Now set the start time to this
        log->set_start_timestamp(data.timestamp());
        // reset it.
        log->set_stop_timestamp(0);
      }
    }

    SymbolSessionLogs[symbol]->set_stop_timestamp(data.timestamp());
  }
}

template <typename T>
static bool WriteDb(const T& value, boost::shared_ptr<historian::Db> db)
{
  bool canOverWrite = !FLAGS_checkDuplicate;
  return db->Write<T>(value, canOverWrite);
}

static bool WriteSessionLogs(const boost::shared_ptr<historian::Db>& db)
{
  bool ok = true;
  for (SessionLogMapIterator itr = SymbolSessionLogs.begin();
       itr != SymbolSessionLogs.end();
       ++itr) {

    const std::string& symbol = itr->first;
    const SessionLog& log = *(itr->second);
    ok = ok && (db->Write(log) > 0);
  }
  return ok;
}

} // namespace utils
} // namespace atp

using namespace ib::internal;

class PublisherSocket : public EWrapperEventCollector
{
 public:
  PublisherSocket(zmq::socket_t* socket) : socket_(socket) {}

  virtual zmq::socket_t* getOutboundSocket(int channel = 0)
  { return socket_; }

 private:
  zmq::socket_t* socket_;
};

class MarketDataPublisher : public EventDispatcherBase
{
 public:

  MarketDataPublisher(zmq::socket_t* socket) :
      EventDispatcherBase(collector_),
      collector_(socket)
  {
  }

 private:
  PublisherSocket collector_;
};

////////////////////////////////////////////////////////
//
// MAIN
//
int main(int argc, char** argv)
{
  google::SetUsageMessage("Reads and publishes market data from logfile.");
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  std::cin.sync_with_stdio(false);

  using namespace boost::posix_time;
  ptime epoch(boost::gregorian::date(1970,boost::gregorian::Jan,1));
  time_facet facet("%Y-%m-%d %H:%M:%S%F%Q");
  std::cout.imbue(std::locale(std::cout.getloc(), &facet));

  boost::shared_ptr<historian::Db> db;
  if (FLAGS_leveldb != NO_VALUE) {
    LOG_READER_LOGGER << "LevelDb on, file = " << FLAGS_leveldb;

    db.reset(new historian::Db(FLAGS_leveldb));
    assert(db->open());
  }

  MarketDataPublisher* publisher = NULL;
  zmq::context_t* context = NULL;
  zmq::socket_t* socket = NULL;
  if (!FLAGS_publish) {
    std::cout << "\"timestamp_utc\",\"symbol\",\"event\",\"value\""
              << std::endl;
  } else {
    context = new zmq::context_t(1);
    socket = new zmq::socket_t(*context, ZMQ_PUSH);
    socket->connect(FLAGS_endpoint.c_str());
    publisher = new MarketDataPublisher(socket);
    LOG(INFO) << "Pushing events to " << FLAGS_endpoint;
  }


  std::vector<string> logfiles;
  boost::split(logfiles, FLAGS_logfile, boost::is_any_of(","));

  LOG(INFO) << "Files = " << logfiles.size();

  for (std::vector<string>::const_iterator logfile = logfiles.begin();
       logfile != logfiles.end();
       ++logfile) {

    const std::string filename = *logfile;

    LOG(INFO) << "Opening file " << filename << endl;

    boost::iostreams::filtering_istream infile;
    bool isCompressed = boost::algorithm::find_first(filename, ".gz");
    if (isCompressed) {
      infile.push(boost::iostreams::gzip_decompressor());
    }
    infile.push(boost::iostreams::file_source(filename));

    if (!infile) {
      LOG(ERROR) << "Unable to open " << filename << endl;
      return -1;
    }

    std::string line;
    int lines = 0;
    int matchedRecords = 0;
    int dbWrittenRecords = 0;
    int dbDuplicateRecords = 0;

    boost::int64_t last_ts = 0;

    // The lines are space separated, so we need to skip whitespaces.
    while (infile >> std::skipws >> line) {
      if (line.find(',') != std::string::npos) {
        LOG_READER_DEBUG << "Log entry = " << line << endl;

        std::map<std::string, std::string> nv; // basic name /value pair

        if (atp::utils::ParseMap(line, nv, '=', ",")) {

          // Check for regular market data event
          historian::MarketData event;
          if (atp::utils::checkEvent(nv) && atp::utils::Convert(nv, &event)) {

            ptime t = historian::as_ptime(event.timestamp());
            bool ext = historian::checkEXT(t);
            if (!ext) {
              continue; // outside trading hours
            }

            bool rth = historian::checkRTH(t);

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
                                                 event.value().double_value());
              size_t sent = marketData.dispatch(socket);
              //std::cerr << event.symbol << " " << sent << std::endl;
              last_ts = event.timestamp();

            } else {

              if (FLAGS_csv) {
                std::cout << t << ","
                          << event.symbol() << ","
                          << event.event() << ",";
                switch (event.value().type()) {
                  case (proto::common::Value_Type_INT) :
                    std::cout << event.value().int_value();
                    break;
                  case (proto::common::Value_Type_DOUBLE) :
                    std::cout << event.value().double_value();
                    break;
                  case (proto::common::Value_Type_STRING) :
                    std::cout << event.value().string_value();
                    break;
                }
                std::cout << std::endl;
              }
            }

            if (db.get() != NULL) {
              bool written = atp::utils::WriteDb(event, db);
              if (written) {
                dbWrittenRecords++;
                LOG_READER_LOGGER << "Db written " << written
                                  << event.ByteSize();
              } else {
                dbDuplicateRecords++;
              }
            }
            matchedRecords++;

          } else if (atp::utils::checkBookEvent(nv)) {

            historian::MarketDepth event;
            if (atp::utils::Convert(nv, &event)) {

              ptime t = historian::as_ptime(event.timestamp());

              bool ext = historian::checkEXT(t);
              if (!ext) {
                continue; // outside trading hours
              }

              bool rth = historian::checkRTH(t);

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

                atp::MarketDepth marketDepth(event.symbol(),
                                             event.timestamp(),
                                             event.side(),
                                             event.level(),
                                             event.operation(),
                                             event.price(),
                                             event.size(),
                                             event.mm());
                size_t sent = marketDepth.dispatch(socket);
                last_ts = event.timestamp();

              } else {

                if (FLAGS_csv) {
                  std::cout << t << ","
                            << event.symbol() << ","
                            << event.side() << ","
                            << event.level() << ","
                            << event.operation() << ","
                            << event.price() << ","
                            << event.size() << ","
                            << event.mm() << std::endl;
                }
              }

              if (db.get() != NULL) {
                bool written = atp::utils::WriteDb(event, db);
                if (written) {
                  dbWrittenRecords++;
                  LOG_READER_LOGGER << "Db written " << written
                                    << event.ByteSize();
                } else {
                  dbDuplicateRecords++;
                }
              }

              matchedRecords++;

            } else {
              LOG_READER_LOGGER << "Parsing failed";
            }

          } else if (atp::utils::checkAction(nv)) {
            // Handle action here.
            atp::utils::MapActions(nv);
          }
        }
        lines++;
      }
    }
    LOG_READER_LOGGER << "Processed " << lines << " lines with "
                      << matchedRecords << " records." << std::endl;
    LOG_READER_LOGGER << "Finishing up -- closing file." << std::endl;
    //infile.close();
    infile.clear();

    if (db.get() != NULL) {
      LOG_READER_LOGGER << "Written: " << dbWrittenRecords << ", Duplicate: " <<
          dbDuplicateRecords;
      // Now write the session logs:
      if (!atp::utils::WriteSessionLogs(db)) {
        LOG(ERROR) << "Falied to write session log!";
      }
    }
  } // for each file


  // Clean up
  if (FLAGS_publish) {
    delete socket;
    delete context;
  }

  LOG_READER_LOGGER << "Completed." << std::endl;
  return 0;
}



