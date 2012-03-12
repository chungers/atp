
#include <iostream>
#include <string>

#include <boost/date_time/posix_time/posix_time.hpp>

#include "proto/common.hpp"
#include "proto/historian.hpp"
#include "historian/time_utils.hpp"


// This file contains the instantiations of templates.

namespace proto {
namespace historian {

using proto::ib::MarketData;
using proto::ib::MarketDepth;
using proto::historian::IndexedValue;
using proto::historian::SessionLog;
using proto::historian::Record;

template inline const Record wrap<MarketData>(const MarketData& v);
template inline const Record wrap<MarketDepth>(const MarketDepth& v);
template inline const Record wrap<SessionLog>(const SessionLog& v);
template inline const Record wrap<IndexedValue>(const IndexedValue& v);

} // historian
} // proto


namespace historian {

using boost::posix_time::ptime;
using proto::common::Value;
using proto::ib::MarketData;
using proto::ib::MarketDepth;
using proto::historian::IndexedValue;
using proto::historian::SessionLog;
using proto::historian::Record;
using namespace proto::common;
using namespace proto::historian;


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
} // historian


