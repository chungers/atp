
#include <boost/date_time/posix_time/posix_time.hpp>

#include "historian/time_utils.hpp"

#include "proto/ostream.hpp"



using boost::posix_time::ptime;
using proto::common::Date;
using proto::common::DateTime;
using proto::common::Time;
using proto::common::Money;
using proto::common::Value;
using proto::ib::MarketData;
using proto::ib::MarketDepth;
using proto::ib::Order;
using proto::ib::OrderStatus;
using proto::historian::IndexedValue;
using proto::historian::SessionLog;
using proto::historian::Record;
using namespace proto::common;
using namespace proto::historian;
using namespace historian;

namespace proto {
namespace common {
std::ostream& format(std::ostream& out, const int v)
{
  if (v > 9) { out << '0' << v; }
  else { out << v; }
  return out;
}
} // common
} // proto

std::ostream& operator<<(std::ostream& out, const Date& v)
{
  using namespace proto::common;
  out << v.year() << '-';
  format(out, v.month()) << '-';
  format(out, v.day());
  return out;
}

std::ostream& operator<<(std::ostream& out, const Time& v)
{
  using namespace proto::common;
  out << v.hour() << ':';
  format(out, v.minute()) << ':';
  format(out, v.second());
  return out;
}

std::ostream& operator<<(std::ostream& out, const DateTime& v)
{
  using namespace proto::common;
  out << v.date() << '-' << v.time();
  return out;
}

std::ostream& operator<<(std::ostream& out, const Money& v)
{
  using namespace proto::common;
  out << v.amount() << '(';
  switch (v.currency()) {
    case Money::USD :
      out << "USD";
      break;
  }
  out << ')';
  return out;
}

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
  ptime t =to_est(as_ptime(v.timestamp()));
  out << t << "," << v.symbol() << "," << v.event() << "," << v.value();
  return out;
}

std::ostream& operator<<(std::ostream& out, const MarketDepth& v)
{
  ptime t = to_est(as_ptime(v.timestamp()));
  out << t << "," << v.symbol() << ","
      << v.operation() << "," << v.level() << ","
      << v.side() << "," << v.price() << ","
      << v.size();
  return out;
}

std::ostream& operator<<(std::ostream& out, const IndexedValue& iv)
{
  using namespace proto::common;
  ptime t =to_est(as_ptime(iv.timestamp()));
  out << t << "," << iv.value();
  return out;
}

std::ostream& operator<<(std::ostream& out, const SessionLog& log)
{
  using namespace historian;
  using namespace proto::common;
  ptime t1 = to_est(as_ptime(log.start_timestamp()));
  ptime t2 = to_est(as_ptime(log.start_timestamp()));

  out << log.symbol() << ","
      << "start=" << t1 << ","
      << "end=" << t2 << ","
      << "source=" << log.source();
  return out;
}

std::ostream& operator<<(std::ostream& out, const QueryByRange& q)
{
  using namespace historian;
  using namespace proto::common;
  out << "Query[" << q.first() << "," << q.last() << ")"
      << ", index=" << q.index();
  return out;
}

std::ostream& operator<<(std::ostream& out, const QueryBySymbol& q)
{
  using namespace historian;
  using namespace proto::common;
  ptime t1 = to_est(as_ptime(q.utc_first_micros()));
  ptime t2 = to_est(as_ptime(q.utc_last_micros()));
  out << "Query[symbol=" << q.symbol() << ","
      << "start=" << t1 << ","
      << "stop=" << t2 << ","
      << "index=" << q.index() << "]";
  return out;
}

std::ostream& operator<<(std::ostream& os, const OrderStatus& o)
{
  os << "OrderStatus="
     << "timestamp:" << o.timestamp()
     << ",message_id:" << o.message_id()
     << ",order_id:" << o.order_id()
     << ",status:" << o.status()
     << ",filled:" << o.filled()
     << ",remaining:" << o.remaining()
     << ",avg_fill_price:" << o.avg_fill_price()
     << ",last_fill_price:" << o.last_fill_price()
     << ",client_id:" << o.client_id()
     << ",why_held:" << o.why_held();

  return os;
}

