#ifndef PROTO_HISTORIAN_H_
#define PROTO_HISTORIAN_H_

#include <string>
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>

#include "proto/common.hpp"
#include "proto/common.pb.h"
#include "proto/ib.pb.h"
#include "proto/historian.pb.h"


// Utilites

namespace proto {
namespace historian {

using std::string;
using boost::optional;
using proto::common::Value;
using proto::ib::MarketData;
using proto::ib::MarketDepth;
using proto::historian::SessionLog;

namespace internal {

template <typename T>
inline optional<T> get(const Record& record)
{
  return optional<T>();
}

template <>
inline optional<MarketData> get<MarketData>(const Record& record)
{
  return optional<MarketData>(record.ib_marketdata());
}

template <>
inline optional<MarketDepth> get<MarketDepth>(const Record& record)
{
  return optional<MarketDepth>(record.ib_marketdepth());
}

template <>
inline optional<SessionLog> get<SessionLog>(const Record& record)
{
  return optional<SessionLog>(record.session_log());
}

template <>
inline optional<Value> get<Value>(const Record& record)
{
  return optional<Value>(record.value());
}

template <typename T>
inline optional<T> as(const proto::historian::Record_Type type,
                      const Record& record)
{
  if (record.type() == type) {
    return get<T>(record);
  } else {
    return optional<T>(); // uninitialized.
  }
}

inline void set_as(const MarketData& v, Record* record)
{
  record->mutable_ib_marketdata()->CopyFrom(v);
}

inline void set_as(const MarketDepth& v, Record* record)
{
  record->mutable_ib_marketdepth()->CopyFrom(v);
}

inline void set_as(const SessionLog& v, Record* record)
{
  record->mutable_session_log()->CopyFrom(v);
}

inline void set_as(const Value& v, Record* record)
{
  record->mutable_value()->CopyFrom(v);
}

template <typename T>
inline const void set_as(proto::historian::Record_Type t,
                         const T& v, Record* record)
{
  record->set_type(t);
  set_as(v, record);
}


} // MarketDepthernal

using namespace internal;

/**
*/
template <typename T> inline optional<T> as(const Record& record)
{
  return optional<T>();
}

/** Returns as MarketData, if Record carries a MarketData. */
template <> inline optional<MarketData> as(const Record& record)
{
  return as<MarketData>(proto::historian::Record_Type_IB_MARKET_DATA, record);
}

/** Returns as MarketDepth, if Record carries a MarketDepth. */
template <> inline optional<MarketDepth> as<MarketDepth>(const Record& record)
{
  return as<MarketDepth>(proto::historian::Record_Type_IB_MARKET_DEPTH, record);
}

/** Returns as SessionLog, if Record carries a SessionLog. */
template <> inline optional<SessionLog> as<SessionLog>(const Record& record)
{
  return as<SessionLog>(proto::historian::Record_Type_SESSION_LOG, record);
}

/** Returns as SessionLog, if Record carries a Value. */
template <> inline optional<Value> as<Value>(const Record& record)
{
  return as<Value>(proto::historian::Record_Type_VALUE, record);
}

template <typename T> inline void set_as(const T& v, Record* record)
{
  // do nothing
}

/** Sets the Record to carry a MarketData. */
template <> inline void set_as<MarketData>(const MarketData& v, Record* record)
{
  set_as<MarketData>(proto::historian::Record_Type_IB_MARKET_DATA, v, record);
}

/** Sets the Record to carry a MarketDepth. */
template <> inline void set_as<MarketDepth>(const MarketDepth& v,
                                            Record* record)
{
  set_as<MarketDepth>(proto::historian::Record_Type_IB_MARKET_DEPTH, v, record);
}

/** Sets the Record to carry a SessionLog. */
template <> inline void set_as<SessionLog>(const SessionLog& v, Record* record)
{
  set_as<SessionLog>(proto::historian::Record_Type_SESSION_LOG, v, record);
}

/** Sets the Record to carry a Value. */
template <> inline void set_as<Value>(const Value& v, Record* record)
{
  set_as<Value>(proto::historian::Record_Type_VALUE, v, record);
}

template <typename T> inline const Record wrap(const T& v)
{
  // Default try to carry it as a Value.
  Value value = proto::common::wrap<T>(v);
  Record record;
  proto::historian::set_as<Value>(value, &record);
  return record;
}

/** Wraps a MarketData in a copy of Record */
template <> inline const Record wrap<MarketData>(const MarketData& v)
{
  Record record; proto::historian::set_as(v, &record); return record;
}

/** Wraps a MarketDepth in a copy of Record */
template <> inline const Record wrap<MarketDepth>(const MarketDepth& v)
{
  Record record; proto::historian::set_as(v, &record); return record;
}

/** Wraps a SessionLog in a copy of Record */
template <> inline const Record wrap<SessionLog>(const SessionLog& v)
{
  Record record; proto::historian::set_as(v, &record); return record;
}

/** Wraps a Value in a copy of Record */
template <> inline const Record wrap<Value>(const Value& v)
{
  Record record; proto::historian::set_as(v, &record); return record;
}


} // historian
} // proto

#endif //PROTO_HISTORIAN_H_

