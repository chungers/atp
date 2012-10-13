#ifndef ATP_SERVICE_LOG_READER_H_
#define ATP_SERVICE_LOG_READER_H_

#include <iostream>
#include <fstream>
#include <string>
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
#include <boost/lexical_cast.hpp>
#include <boost/unordered_map.hpp>

#include <glog/logging.h>

#include "ib/contract_symbol.hpp"

#include "log_levels.h"
#include "proto/ib.pb.h"
#include "historian/time_utils.hpp"


using namespace std;
using namespace boost::gregorian;
using namespace boost::algorithm;
using namespace boost::iostreams;
using namespace boost::posix_time;

typedef boost::uint64_t timer_t;




namespace atp {
namespace log_reader {

typedef boost::unordered_map<string, string> log_record_t;



namespace internal {

using namespace historian;

ostream& operator<<(ostream& stream, const log_record_t& r);

// parse a single log line into a map of name/value pairs
static bool line_to_record(const string& line,
                           log_record_t& result,
                           const char nvDelim,
                           const string& pDelim) {
  // Split by the '=' into name-value pairs:
  vector<string> nvpairs_vec;
  boost::split(nvpairs_vec, line, boost::is_any_of(pDelim));
  if (nvpairs_vec.empty()) {
    return false;
  }

  // Split the name-value pairs by '=' or nvDelim into a map:
  vector<string>::const_iterator itr;
  for (itr = nvpairs_vec.begin(); itr != nvpairs_vec.end(); ++itr) {
    int sep = itr->find(nvDelim);
    string key = itr->substr(0, sep);
    string val = itr->substr(sep + 1, itr->length() - sep);
    result[key] = val;
  }
  return true;
}

////////////////////////////////////////////////
/// Event types that contain values
///
/// Supported event types and their value type for conversion from log text.
typedef pair<string, proto::common::Value_Type> event_type;
typedef boost::unordered_map<string, event_type> event_value_type_map_t;

static event_value_type_map_t VALUE_EVENTS =
    boost::assign::map_list_of
    ("tickPrice", event_type("price", proto::common::Value_Type_DOUBLE))
    ("tickSize", event_type("size", proto::common::Value_Type_INT))
    ("tickGeneric", event_type("value", proto::common::Value_Type_STRING))
    ;

static bool get_event_type(const string& event, event_type* out)
{
  event_value_type_map_t::const_iterator found = VALUE_EVENTS.find(event);
  if (found == VALUE_EVENTS.end()) {
    return false;
  }
  *out = found->second;
  return true;;
}

static bool is_supported(const log_record_t& event, map<string, size_t>& count)
{
  typedef map<string, size_t>::iterator counter;

  log_record_t::const_iterator found = event.find("event");
  if (found != event.end()) {
    bool supported = VALUE_EVENTS.find(found->second) != VALUE_EVENTS.end();
    if (supported) {
      return true;
    }

    counter counter = count.find(found->second);
    if (counter != count.end()) {
      counter->second += 1;
    } else {
      count[found->second] = 0;
    }
  }
  return false;
}

////////////////////////////////////////////////
/// Actions that contain reference data
///
typedef boost::unordered_map<string, string> action_field_map_t;
static action_field_map_t ACTIONS = boost::assign::map_list_of
    ("reqMktData", "contract")
    ("reqMktDepth", "contract")
    ("reqRealTimeBars", "contract")
    ("reqHistoricalData", "contract")
    ;
static bool is_action(const log_record_t& event)
{
  log_record_t::const_iterator found = event.find("action");
  if (found != event.end()) {
    return ACTIONS.find(found->second) != ACTIONS.end();
  }
  return false;
}


////////////////////////////////////////////////
/// Mapping of ticker id to symbols
///
typedef boost::unordered_map<long,string> ticker_id_symbol_map_t;

ticker_id_symbol_map_t& symbol_map();
ostream& operator<<(ostream& stream, const ticker_id_symbol_map_t& m);

static bool code_to_symbol(const long& code, string* symbol)
{
  ticker_id_symbol_map_t& ticker_id_symbol_map = symbol_map();
  ticker_id_symbol_map_t::const_iterator found =
      ticker_id_symbol_map.find(code);
  if (found == ticker_id_symbol_map.end()) {
    return false;
  }
  *symbol = found->second;
  return true;
}

static bool map_id_to_symbols(const log_record_t& nv)
{
  ticker_id_symbol_map_t& ticker_id_symbol_map = symbol_map();
  log_record_t::const_iterator found_id = nv.find("id");
  if (found_id == nv.end()) {
    return false;
  }

  long tickerId = boost::lexical_cast<long>(found_id->second);
  string symbol;
  string contractString;

  log_record_t::const_iterator found_contract = nv.find("contract");
  if (found_contract != nv.end()) {

    contractString = found_contract->second;

    // Build a symbol string from the contract spec.
    // This is a ; separate list of name:value pairs.
    log_record_t nv; // basic name /value pair

    if (internal::line_to_record(contractString, nv, ':', ";")) {

      bool ok = ib::internal::symbol_from_contract(nv, &symbol);
      if (!ok) {
        LOG(FATAL) << "Failed to generate symbol from parsed contract spec: "
                   << contractString;
      }

      if (ticker_id_symbol_map.find(tickerId) ==
          ticker_id_symbol_map.end()) {

        ticker_id_symbol_map[tickerId] = symbol;
        LOG(INFO) << "Ticker id " << tickerId << " ==> " << symbol;
        return true;

      } else {

        // We may have a collision!
        if (ticker_id_symbol_map[tickerId] != symbol) {
          LOG(FATAL) << "collision for " << tickerId << ": "
                     << symbol << " and " << ticker_id_symbol_map[tickerId];
          return false;
        }

      }

    } else {
      LOG(FATAL) << "Failed to parse contract spec: " << contractString;
    }
  }
  return false;
}


static bool check_time(const log_record_t& event, bool regular_trading_hours)
{
  log_record_t::const_iterator found = event.find("ts_utc");
  if (found == event.end()) {
    return false;
  }
  timer_t timestamp = boost::lexical_cast<timer_t>(found->second);
  ptime t = historian::as_ptime(timestamp);
  bool ext = historian::checkEXT(t);
  if (!ext) {
    return false; // outside trading hours
  }
  bool rth = historian::checkRTH(t);
  if (!rth && regular_trading_hours) {
    // Skip if not RTH and we want only data during trading hours.
    return false;
  }
  return true;
}

}; // internal





/////////////////////////////////////////////////////////////
/// Marshallers - copying data from map to proto
///
namespace marshall {

namespace p = proto::ib;

bool operator<<(p::MarketData& p, const log_record_t& nv);
bool operator<<(p::MarketDepth& p, const log_record_t& nv);

template <typename T>
inline bool operator>>(const log_record_t& nv, T& p)
{ return p << nv; }

} // marshall





/////////////////////////////////////////////////////////////
/// Simple logfile reader
class LogReader
{
 public:

  typedef boost::unordered_map<long,string> ticker_id_symbol_map_t;

  LogReader(const string& logfile,
            bool rth = true,
            bool est = true,
            bool sync_stdio = false) :
      logfile_(logfile), rth_(rth), est_(est)
  {
    std::cin.sync_with_stdio(sync_stdio);
  }

  size_t Process()
  {
    namespace p = proto::ib;
    using namespace marshall;

    LOG(INFO) << "Opening file " << logfile_;

    filtering_istream infile;
    bool isCompressed = find_first(logfile_, ".gz");
    if (isCompressed) {
      infile.push(gzip_decompressor());
    }
    infile.push(file_source(logfile_));

    if (!infile) {
      LOG(ERROR) << "Unable to open " << logfile_ << endl;
      return 0;
    }

    std::string line;
    size_t lines = 0;
    size_t matchedRecords = 0;

    timer_t last_ts = 0;
    timer_t last_log = 0;
    int last_written = 0;
    ptime last_log_t;

    // The lines are space separated, so we need to skip whitespaces.
    while (infile >> std::skipws >> line) {
      if (line.find(',') != std::string::npos) {

        LOG_READER_DEBUG << "Log entry = " << line;

        // parse the log line text into record
        log_record_t nv; // basic name /value pair
        if (!internal::line_to_record(line, nv, '=', ",")) {
          LOG(ERROR) << "unable to parse: " << line;
          continue;
        }

        // constructs any mapping of symbols and ids
        // this comes from actions the client made (eg. requestMktData)
        if (internal::is_action(nv) && internal::map_id_to_symbols(nv)) {
          continue;
        }

        if (!internal::is_supported(nv, unsupported_events_)) {
          continue;
        }

        if (!internal::check_time(nv, rth_)) {
          continue;
        }

        p::MarketData marketdata;
        if (nv >> marketdata) {

          matchedRecords++;

          ptime t = historian::as_ptime(marketdata.timestamp());
          if (last_log_t == boost::posix_time::not_a_date_time) {
            last_log_t = t;
            last_log = now_micros();
          } else {
            time_duration dt = t - last_log_t;
            timer_t now = now_micros();
            timer_t elapsed = now - last_log;
            double elapsedSec = static_cast<double>(elapsed) / 1000000.;
            if (dt.minutes() >= 15) {
              LOG(INFO) << "Currently at " << historian::to_est(t)
                        << ": in " << elapsedSec << " sec. "
                        << ": matchedRecords=" << matchedRecords;
              last_log_t = t;
              last_log = now;
            }
          }

          // Do something with the parsed marketdata
          cout << ((est_) ? historian::to_est(t) : t) << ","
               << marketdata.symbol() << ","
               << marketdata.event() << ",";
          switch (marketdata.value().type()) {
            case (proto::common::Value_Type_INT) :
              cout << marketdata.value().int_value();
              break;
            case (proto::common::Value_Type_DOUBLE) :
              cout << marketdata.value().double_value();
              break;
            case (proto::common::Value_Type_STRING) :
              cout << marketdata.value().string_value();
              break;
          }
          cout << endl;

          lines++;
          continue;
        }

        p::MarketDepth marketdepth;
        if (nv >> marketdepth) {

          matchedRecords++;

          ptime t = historian::as_ptime(marketdepth.timestamp());

          if (last_log_t == boost::posix_time::not_a_date_time) {
            last_log_t = t;
            last_log = now_micros();
          } else {
            time_duration dt = t - last_log_t;
            timer_t now = now_micros();
            timer_t elapsed = now - last_log;
            double elapsedSec = static_cast<double>(elapsed) / 1000000.;
            if (dt.minutes() >= 15) {
              LOG(INFO) << "Currently at " << historian::to_est(t)
                        << ": in " << elapsedSec << " sec. "
                        << ": matchedRecords=" << matchedRecords;
              last_log_t = t;
              last_log = now;
            }
          }

          // Do something with the marketdepth
          cout << ((est_) ? historian::to_est(t) : t) << ","
               << marketdepth.symbol() << ","
               << marketdepth.side() << ","
               << marketdepth.level() << ","
               << marketdepth.operation() << ","
               << marketdepth.price() << ","
               << marketdepth.size() << ","
               << marketdepth.mm() << endl;

          lines++;
          continue;
        }

        LOG_READER_LOGGER << "Skipping: " << line;
      }
    }
    LOG(INFO) << "Processed " << lines << " lines with "
              << matchedRecords << " records.";
    LOG_READER_LOGGER << "Finishing up -- closing file.";
    infile.clear();

    return matchedRecords;
  }


 private:
  string logfile_;
  bool rth_;
  bool est_;
  map<string, size_t> unsupported_events_;
};


} // service
} // atp

#endif //ATP_SERVICE_LOG_READER_H_
