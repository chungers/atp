
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>

#include <boost/algorithm/string.hpp>
#include "boost/assign.hpp"
#include <boost/date_time/gregorian/greg_month.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <boost/format.hpp>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "log_levels.h"
#include "ib/ticker_id.hpp"
#include "ib/tick_types.hpp"
#include "ib/EventDispatcherBase.hpp"
#include "ib/TickerMap.hpp"
#include "zmq/ZmqUtils.hpp"

DEFINE_bool(rth, true, "Regular trading hours");
DEFINE_bool(delay, true, "True to simulate sample delays when publishing");
DEFINE_string(logfile, "logfile", "The name of the log file.");
DEFINE_string(endpoint, "tcp://127.0.0.1:5555", "Endpoint for publishing.");
DEFINE_bool(publish, true, "True to publish to endpoint.");

namespace atp {
namespace utils {


std::map<std::string, std::string> ACTIONS = boost::assign::map_list_of
    ("reqMktData", "contract")
    ("reqMktDepth", "contract")
    ("reqRealTimeBars", "contract")
    ("reqHistoricalData", "contract")
    ;

std::map<std::string, std::string> EVENTS = boost::assign::map_list_of
    ("tickPrice", "price")
    ("tickSize", "size")
    ("tickGeneric", "value")
    ;

template <typename T>
struct Event
{
  string symbol;
  string event;
  boost::int64_t ts;
  T value;
};

static std::map<long,std::string> TickerIdSymbolMap;

static void GetSymbol(long code, std::string* symbol)
{
  if (TickerIdSymbolMap.find(code) == TickerIdSymbolMap.end()) {
    LOG(WARNING) << "No mapping exists for ticker id " << code;
    std::string sym;
    ib::internal::SymbolFromTickerId(code, &sym);
    TickerIdSymbolMap[code] = sym + ".STK";
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
        TickerIdSymbolMap[tickerId] = symbol;
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
                    Event<double>* result)
{
  uint64_t ts = 0;
  std::string symbol;
  std::string event;
  double value;

  ////////// Timestamp
  if (!GetField(nv, "ts_utc", &ts)) {
    return false;
  }
  LOG_READER_DEBUG << "Timestamp ==> " << ts << endl;

  ////////// TickerId / Symbol:
  long code = 0;
  if (!GetField(nv, "tickerId", &code)) {
    return false;
  }
  GetSymbol(code, &symbol);
  nv["symbol"] = symbol;
  LOG_READER_DEBUG << "symbol ==> " << symbol << "(" << code << ")" << endl;

  ////////// Event
  if (!GetField(nv, "field", &event)) {
    return false;
  }
  LOG_READER_DEBUG << "event ==> " << event << endl;

  ////////// Price / Size
  std::string method;
  if (GetField(nv, "event", &method)) {
    std::string fieldToUse = EVENTS[method];
    if (!GetField(nv, fieldToUse, &value)) {
      return false;
    }
  }

  LOG_READER_DEBUG << "value ==> " << value << endl;

  LOG_READER_DEBUG
      << symbol << ' '
      << event << ' '
      << ts << ' '
      << value << endl;

  result->ts = ts;
  result->symbol = symbol;
  result->event = event;
  result->value = value;
  return true;
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
      collector_(socket),
      EventDispatcherBase(collector_)
  {
  }

 private:
  PublisherSocket collector_;
};

static boost::posix_time::time_duration RTH_START(9, 30, 0, 0);
static boost::posix_time::time_duration RTH_END(16, 0, 0, 0);

////////////////////////////////////////////////////////
//
// MAIN
//
int main(int argc, char** argv)
{
  google::SetUsageMessage("Reads and publishes market data from logfile.");
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  const std::string filename = FLAGS_logfile;

  // Open the file inputstream
  LOG_READER_LOGGER << "Opening file " << filename << endl;

  std::ifstream infile(filename.c_str());

  if (!infile) {
    LOG(ERROR) << "Unable to open " << filename << endl;
    return -1;
  }


  std::string line;
  int lines = 0;
  int matchedRecords = 0;

  boost::posix_time::time_facet facet("%Y-%m-%d %H:%M:%S%F%Q");

  std::cout.imbue(std::locale(std::cout.getloc(), &facet));

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

  using namespace boost::posix_time;

  ptime epoch(boost::gregorian::date(1970,boost::gregorian::Jan,1));

  boost::int64_t last_ts = 0;

  // The lines are space separated, so we need to skip whitespaces.
  while (infile >> std::skipws >> line) {
    if (line.find(',') != std::string::npos) {
      LOG_READER_DEBUG << "Log entry = " << line << endl;

      std::map<std::string, std::string> nv; // basic name /value pair
      atp::utils::Event<double> event; // market data event in EVENTS

      if (atp::utils::ParseMap(line, nv, '=', ",")) {
        if (atp::utils::checkEvent(nv) && atp::utils::Convert(nv, &event)) {

          boost::posix_time::ptime t = boost::posix_time::from_time_t(
              event.ts / 1000000LL);
          boost::posix_time::time_duration micros(
              0, 0, 0, event.ts % 1000000LL);
          t += micros;

          boost::posix_time::time_duration eastern =
              us_eastern::utc_to_local(t).time_of_day();

          bool rth = eastern >= RTH_START and eastern < RTH_END;
          if (!rth && FLAGS_rth) {
            // Skip if not RTH and we want only data during trading hours.
            continue;
          }
          if (FLAGS_publish) {

            boost::int64_t dt = event.ts - last_ts;
            if (last_ts > 0 && dt > 0 && FLAGS_delay) {
              // wait dt micros
              usleep(dt);
            }
            atp::MarketData<double> marketData(event.symbol, event.ts,
                                               event.event, event.value);
            size_t sent = marketData.dispatch(socket);
            //std::cerr << event.symbol << " " << sent << std::endl;
            last_ts = event.ts;

          } else {

            std::cout << t << ","
                      << event.symbol << ","
                      << event.event << ","
                      << event.value << std::endl;
          }

          matchedRecords++;

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
  infile.close();
  infile.clear();

  if (FLAGS_publish) {
    delete socket;
    delete context;
  }
  LOG_READER_LOGGER << "Completed." << std::endl;
  return 0;
}



