
#include "log_levels.h"
#include "service/LogReader.hpp"

namespace atp {
namespace log_reader {

namespace internal {

ticker_id_symbol_map_t& symbol_map()
{
  static ticker_id_symbol_map_t m;
  return m;
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

};

namespace marshall {

namespace p = proto::ib;
using namespace std;

typedef boost::uint64_t timer_t;


namespace internal {

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

} // internal


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
  double value;
  u::event_type ft;
  if (internal::get_field(nv, "event", &method)) {
    if (!u::get_event_type(method, &ft)) {
      LOG_READER_DEBUG << "method ==> " << method;
      return false;
    }

    string fieldToUse = ft.first;
    Value_Type type = ft.second;

    if (!internal::get_field(nv, fieldToUse, &value)) {
      LOG(FATAL) << "no field to use " << fieldToUse;
      return false;
    }
    result.mutable_value()->set_type(type);
    switch (type) {
      case proto::common::Value_Type_INT :
        result.mutable_value()->set_int_value(value);
        break;
      case proto::common::Value_Type_DOUBLE :
        result.mutable_value()->set_double_value(value);
        break;
      case proto::common::Value_Type_STRING :
        result.mutable_value()->set_string_value(""); // TODO
        break;
      default:
        return false;
    }
  }
  LOG_READER_DEBUG << "value ==> " << result.value().double_value();

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

} // marshall
} // log_reader
} // atp