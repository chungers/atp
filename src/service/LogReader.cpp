
#include "log_levels.h"
#include "service/LogReaderInternal.hpp"
#include "service/LogReader.hpp"

namespace atp {
namespace log_reader {
namespace internal {

namespace p = proto::ib;
using namespace std;
typedef boost::uint64_t timer_t;

using atp::platform::types::timestamp_t;


ticker_id_symbol_map_t& symbol_map()
{
  static ticker_id_symbol_map_t m;
  return m;
}

const action_field_map_t& action_map()
{
  static action_field_map_t map = boost::assign::map_list_of
      ("reqMktData", "contract")
      ("reqMktDepth", "contract")
      ("reqRealTimeBars", "contract")
      ("reqHistoricalData", "contract")
      ;
  return map;
}

const event_value_type_map_t& event_value_map()
{
  using namespace proto::common;

  static event_value_type_map_t map =
      boost::assign::map_list_of
      ("tickPrice", event_type("price", Value_Type_DOUBLE))
      ("tickPrice/CLOSE", event_type("price", Value_Type_DOUBLE))
      ("tickPrice/BID", event_type("price", Value_Type_DOUBLE))
      ("tickPrice/ASK", event_type("price", Value_Type_DOUBLE))
      ("tickPrice/LAST", event_type("price", Value_Type_DOUBLE))

      ("tickSize", event_type("size", Value_Type_INT))
      ("tickSize/BID_SIZE", event_type("size", Value_Type_INT))
      ("tickSize/ASK_SIZE", event_type("size", Value_Type_INT))
      ("tickSize/LAST_SIZE", event_type("size", Value_Type_INT))

      ("tickString", event_type("value", Value_Type_STRING))
      ("tickString/LAST_TIMESTAMP", event_type("value", Value_Type_TIMESTAMP))

      // ("tickGeneric", event_type("value", Value_Type_STRING))

      ;
  return map;
}

static string stringify(const log_record_t& r)
{
  ostringstream stream;
  log_record_t::const_iterator itr = r.begin();
  if (r.size() == 0) {
    stream << "{}";
    return stream.str();
  }
  stream << "{";
  stream << '"' << itr->first << "\":"
         << '"' << itr->second << '"';
  ++itr;

  for (; itr != r.end(); ++itr) {
    stream << ", \"" << itr->first << "\":"
           << '"' << itr->second << '"';
  }
  stream << "}[" << r.size() << "]";
  return stream.str();
}

ostream& operator<<(ostream& stream, const log_record_t& r)
{
  stream << stringify(r);
  return stream;
}

ostream& operator<<(ostream& stream, const ticker_id_symbol_map_t& m)
{
  for (ticker_id_symbol_map_t::const_iterator itr = m.begin();
       itr != m.end();
       ++itr) {
    stream << itr->first << ',' << itr->second << endl;
  }
  return stream;
}

template <typename T>
bool get_field(const log_record_t& nv, const string& key, T* value)
{
  log_record_t::const_iterator found = nv.find(key);
  if (found != nv.end()) {
    *value = boost::lexical_cast<T>(found->second);
    return true;
  }
  return false;
}

bool operator<<(p::MarketData& result, const log_record_t& nv)
{
  using namespace atp::log_reader::internal;

  //LOG(INFO) << stringify(nv);

  namespace u = atp::log_reader::internal;

  timer_t ts = 0;
  ////////// Timestamp
  if (!internal::get_field(nv, "ts_utc", &ts)) {
    return false;
  }
  result.set_timestamp(ts);
  LOG_READER_DEBUG << "Timestamp ==> " << result.timestamp();

  ////////// TickerId / Symbol:
  long code = 0;
  if (!internal::get_field(nv, "tickerId", &code)) {
    LOG(FATAL) << "No tickerId!";
    return false;
  }

  ////////// Symbol / contract id
  string symbol;
  if (!u::code_to_symbol(code, &symbol)) {
    LOG_READER_DEBUG << "Unknow tickerId = " << code;
    LOG(FATAL) << "bad tickerid " << code;
    return false;
  }
  result.set_contract_id(code);

  result.set_symbol(symbol);
  LOG_READER_DEBUG << "symbol ==> " << result.symbol()
                   << "(" << code << ")";

  ////////// Event
  string event;
  if (!internal::get_field(nv, "field", &event)) {
    LOG(FATAL) << "no field";
    return false;
  }
  result.set_event(event);
  LOG_READER_DEBUG << "event ==> " << result.event();

  ////////// Price / Size
  using proto::common::Value_Type;

  string method;
  u::event_type ft;
  if (internal::get_field(nv, "event", &method)) {
    string key(method + '/' + event);
    if (!u::get_event_type(key, &ft)) {
      LOG_READER_DEBUG << "method ==> " << key;
      return false;
    }

    string fieldToUse = ft.first;
    Value_Type type = ft.second;
    result.mutable_value()->set_type(type);
    switch (type) {
      case proto::common::Value_Type_INT :
        {
          int value;
          if (!internal::get_field(nv, fieldToUse, &value)) {
            LOG(FATAL) << "no field to use " << fieldToUse;
            return false;
          }
          result.mutable_value()->set_int_value(value);
        }
        break;
      case proto::common::Value_Type_DOUBLE :
        {
          double value;
          if (!internal::get_field(nv, fieldToUse, &value)) {
            LOG(FATAL) << "no field to use " << fieldToUse;
            return false;
          }
          result.mutable_value()->set_double_value(value);
        }
        break;
      case proto::common::Value_Type_STRING :
        {
          string value;
          if (!internal::get_field(nv, fieldToUse, &value)) {
            LOG(FATAL) << "no field to use " << fieldToUse;
            return false;
          }
          result.mutable_value()->set_string_value(value);
        }
        break;
      case proto::common::Value_Type_TIMESTAMP :
        {
          timestamp_t value;
          if (!internal::get_field(nv, fieldToUse, &value)) {
            LOG(FATAL) << "no field to use " << fieldToUse;
            return false;
          }
          result.mutable_value()->set_timestamp_value(value);
        }
        break;
      default:
        return false;
    }
  }
  return true;
}

bool operator<<(p::MarketDepth& result, const log_record_t& nv)
{
  namespace u = atp::log_reader::internal;

  ////////// Timestamp
  timer_t ts;
  if (!internal::get_field(nv, "ts_utc", &ts)) {
    return false;
  }
  result.set_timestamp(ts);
  LOG_READER_DEBUG << "Timestamp ==> " << result.timestamp();

  ////////// TickerId / Symbol:
  long code = 0;
  if (!internal::get_field(nv, "id", &code)) {
    return false;
  }
  result.set_contract_id(code);

  ////////// Symbol
  string symbol;
  if (!u::code_to_symbol(code, &symbol)) {
    LOG_READER_DEBUG << "Unknow tickerId = " << code;
    return false;
  }
  result.set_symbol(symbol);
  LOG_READER_DEBUG << "symbol ==> " << result.symbol()
                   << "(" << code << ")";

  ////////// Side
  int parsed_side;
  if (!internal::get_field(nv, "side", &parsed_side)) {
    return false;
  }
  switch (parsed_side) {
    case (proto::ib::MarketDepth_Side_ASK) :
      result.set_side(proto::ib::MarketDepth_Side_ASK);
      break;
    case (proto::ib::MarketDepth_Side_BID) :
      result.set_side(proto::ib::MarketDepth_Side_BID);
      break;
  }
  LOG_READER_DEBUG << "side ==> " << result.side();

  ////////// Operation
  int parsed_operation;
  if (!internal::get_field(nv, "operation", &parsed_operation)) {
    return false;
  }
  switch (parsed_operation) {
    case (proto::ib::MarketDepth_Operation_INSERT) :
      result.set_operation(proto::ib::MarketDepth_Operation_INSERT);
      break;
    case (proto::ib::MarketDepth_Operation_DELETE) :
      result.set_operation(proto::ib::MarketDepth_Operation_DELETE);
      break;
    case (proto::ib::MarketDepth_Operation_UPDATE) :
      result.set_operation(proto::ib::MarketDepth_Operation_UPDATE);
      break;
  }
  LOG_READER_DEBUG << "operation ==> " << result.operation();

  ////////// Level
  int parsed_level;
  if (!internal::get_field(nv, "position", &parsed_level)) {
    return false;
  }
  result.set_level(parsed_level);
  LOG_READER_DEBUG << "level ==> " << result.level();

  ////////// Price
  double parsed_price;
  if (!internal::get_field(nv, "price", &parsed_price)) {
    return false;
  }
  result.set_price(parsed_price);
  LOG_READER_DEBUG << "price ==> " << result.price();

  ////////// Size
  int parsed_size;
  if (!internal::get_field(nv, "size", &parsed_size)) {
    return false;
  }
  result.set_size(parsed_size);
  LOG_READER_DEBUG << "size ==> " << result.size();

  ////////// MM (optional)
  string parsed_mm;
  if (!internal::get_field(nv, "marketMaker", &parsed_mm)) {
    result.set_mm("L1");
  }
  result.set_mm(parsed_mm);
  LOG_READER_DEBUG << "mm ==> " << result.mm();
  return true;
}

} // internal


size_t LogReader::Process(marketdata_visitor_t& marketdata_visitor,
                          marketdepth_visitor_t& marketdepth_visitor,
                          const time_duration_t& duration,
                          const time_t& start)
{
  using namespace internal;

  namespace p = proto::ib;

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

  ptime last_log_t;
  ptime first_log_t;
  ptime current_log_t;
  time_duration elapsed_log_t;
  time_duration scan_duration;

  // The lines are space separated, so we need to skip whitespaces.
  while (infile >> std::skipws >> line) {
    if (line.find(',') != std::string::npos) {

      LOG_READER_DEBUG << "log:" << line;

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

      last_log_t = current_log_t;

      if (internal::get_timestamp(nv, &current_log_t)) {
        if (last_log_t == boost::posix_time::not_a_date_time) {
          last_log_t = current_log_t;
        }
        elapsed_log_t = current_log_t - last_log_t;
        if (current_log_t < start) {
          continue; // do not start yet.
        }
      }

      if (first_log_t == boost::posix_time::not_a_date_time &&
          matchedRecords == 1) {
        first_log_t = current_log_t;
      }

      scan_duration = current_log_t - first_log_t;
      if (scan_duration > duration) {
        LOG(INFO) << "Scanned " << duration << ". Stopping.";
        break;
      }

      p::MarketData marketdata;
      if (nv >> marketdata) {
        marketdata_visitor(marketdata);
        matchedRecords++;
        lines++;
        continue;
      }

      p::MarketDepth marketdepth;
      if (nv >> marketdepth) {
        marketdepth_visitor(marketdepth);
        matchedRecords++;
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


} // log_reader
} // atp
