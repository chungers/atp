
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


DEFINE_string(logfile, "logfile", "The name of the log file.");

namespace atp {
namespace utils {


std::map<std::string, std::string> EVENTS = boost::assign::map_list_of
    ("tickPrice", "price")
    ("tickSize", "size")
    ("tickGeneric", "value")
    ;

template <typename T>
struct Event {
  string symbol;
  string event;
  boost::int64_t ts;
  T value;
};

static bool check(std::map<std::string, std::string>& event) {
  std::map<std::string, std::string>::iterator found = event.find("event");
  if (found != event.end()) {
    return EVENTS.find(event["event"]) != EVENTS.end();
  }
  return false;
}

static bool GetField(std::map<std::string, std::string>& nv,
                     std::string key, uint64_t* out) {
  std::map<std::string, std::string>::iterator found = nv.find(key);
  if (found != nv.end()) {
    char* end;
    *out = static_cast<uint64_t>(strtoll(found->second.c_str(), &end, 10));
    return true;
  }
  return false;
}

static bool GetField(std::map<std::string, std::string>& nv,
                     std::string key, std::string* out) {
  std::map<std::string, std::string>::iterator found = nv.find(key);
  if (found != nv.end()) {
    out->assign(nv[key]);
    return true;
  }
  return false;
}

static bool GetField(std::map<std::string, std::string>& nv,
                     std::string key, int* out) {
  std::map<std::string, std::string>::iterator found = nv.find(key);
  if (found != nv.end()) {
    *out = atoi(nv[key].c_str());
    return true;
  }
  return false;
}

static bool GetField(std::map<std::string, std::string>& nv,
                     std::string key, double* out) {
  std::map<std::string, std::string>::iterator found = nv.find(key);
  if (found != nv.end()) {
    *out = atof(nv[key].c_str());
    return true;
  }
  return false;
}

static bool Convert(std::map<std::string, std::string>& nv,
                    Event<double>* result) {
  if (!check(nv)) {
    LOG_READER_DEBUG << "Not a numeric event. " << endl;
    return false;
  }

  uint64_t ts;
  std::string symbol;
  std::string event;
  double value;

  ////////// Timestamp
  if (!GetField(nv, "ts_utc", &ts)) {
    return false;
  }
  LOG_READER_DEBUG << "Timestamp ==> " << ts << endl;

  ////////// TickerId / Symbol:
  int code = 0;
  if (!GetField(nv, "tickerId", &code)) {
    return false;
  }
  ib::internal::SymbolFromTickerId(code, &symbol);
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

inline static bool ParseLine(const std::string& token,
                             std::map<std::string, std::string>& result) {
  // Split by the '=' into name-value pairs:
  vector<std::string> nvpairs_vec;
  boost::split(nvpairs_vec, token, boost::is_any_of(","));

  if (nvpairs_vec.empty()) {
    return false;
  }

  // Split the name-value pairs by '=' into a map:
  vector<std::string>::iterator itr;
  for (itr = nvpairs_vec.begin(); itr != nvpairs_vec.end(); ++itr) {
    int sep = itr->find('=');
    std::string key = itr->substr(0, sep);
    std::string val = itr->substr(sep + 1, itr->length() - sep);
    LOG_READER_DEBUG << "Name-Value Pair: " << *itr
                     << " (" << key << ", " << val << ") " << endl;
    result[key] = val;
  }
  LOG_READER_DEBUG << "Map, size=" << result.size() << endl;
  return true;
}


} // namespace utils
} // namespace atp

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

  boost::posix_time::ptime epoch(
      boost::gregorian::date(1970,boost::gregorian::Jan,1));

  std::cout << "\"timestamp_utc\",\"symbol\",\"event\",\"value\"" << std::endl;


  while (infile >> line) {
    // The lines are space separated
    if (line.find(',') != std::string::npos) {
      LOG_READER_DEBUG << "Log entry = " << line << endl;

      std::map<std::string, std::string> nv; // basic name /value pair
      atp::utils::Event<double> event; // market data event in EVENTS

      if (atp::utils::ParseLine(line, nv) &&
          atp::utils::Convert(nv, &event)) {

        boost::posix_time::ptime t = boost::posix_time::from_time_t(
            event.ts / 1000000LL);
        boost::posix_time::time_duration micros(
            0, 0, 0, event.ts % 1000000LL);

        t += micros;

        std::cout << t << ","
                  << event.symbol << ","
                  << event.event << ","
                  << event.value << std::endl;

        matchedRecords++;
      }
      lines++;
    }
  }
  LOG_READER_LOGGER << "Processed " << lines << " lines with "
                    << matchedRecords << " records." << std::endl;
  LOG_READER_LOGGER << "Finishing up -- closing file." << std::endl;
  infile.close();
  infile.clear();
  LOG_READER_LOGGER << "Completed." << std::endl;
  return 0;
}



