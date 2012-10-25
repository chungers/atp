#ifndef ATP_SERVICE_LOG_READER_INTERNAL_H_
#define ATP_SERVICE_LOG_READER_INTERNAL_H_

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <map>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/assign.hpp>
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

#include "log_levels.h"
#include "ib/contract_symbol.hpp"
#include "proto/ib.pb.h"
#include "historian/time_utils.hpp"


using namespace std;
using namespace boost::gregorian;
using namespace boost::algorithm;
using namespace boost::iostreams;
using namespace boost::posix_time;

typedef boost::unordered_map<string, string> log_record_t;
typedef boost::uint64_t log_timer_t;


namespace atp {
namespace log_reader {
namespace internal {

using namespace historian;
namespace p = proto::ib;

bool operator<<(p::MarketData& p, const log_record_t& nv);
bool operator<<(p::MarketDepth& p, const log_record_t& nv);

template <typename T>
inline bool operator>>(const log_record_t& nv, T& p)
{ return p << nv; }


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

const event_value_type_map_t& event_value_map();

static bool get_event_type(const string& event, event_type* out)
{
  const event_value_type_map_t& map = event_value_map();

  event_value_type_map_t::const_iterator found = map.find(event);
  if (found == map.end()) {
    return false;
  }
  *out = found->second;
  return true;;
}

static bool is_supported(const log_record_t& event, map<string, size_t>& count)
{
  typedef map<string, size_t>::iterator counter;
  const event_value_type_map_t& map = event_value_map();

  log_record_t::const_iterator found = event.find("event");
  if (found != event.end()) {
    bool supported = map.find(found->second) != map.end();
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

const action_field_map_t& action_map();

static bool is_action(const log_record_t& event)
{
  const action_field_map_t& actions = action_map();
  log_record_t::const_iterator found = event.find("action");
  if (found != event.end()) {
    return actions.find(found->second) != actions.end();
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
        LOG_READER_LOGGER << "Ticker id " << tickerId << " ==> " << symbol;
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

static bool get_timestamp(const log_record_t& event, ptime* timestamp)
{
  log_record_t::const_iterator found = event.find("ts_utc");
  if (found == event.end()) {
    return false;
  }
  log_timer_t ts = boost::lexical_cast<log_timer_t>(found->second);
  *timestamp = historian::as_ptime(ts);
  return true;
}

static bool check_time(const log_record_t& event, bool regular_trading_hours)
{
  ptime t;
  if (!get_timestamp(event, &t)) {
    return false;
  }

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


} // internal
} // log_reader
} // atp

#endif //ATP_SERVICE_LOG_READER_INTERNAL_H_
