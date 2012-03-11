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
using proto::historian::IndexedValue;
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
inline optional<IndexedValue> get<IndexedValue>(const Record& record)
{
  return optional<IndexedValue>(record.indexed_value());
}

template <typename T>
inline optional<T> as(const proto::historian::Type type,
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

inline void set_as(const IndexedValue& v, Record* record)
{
  record->mutable_indexed_value()->CopyFrom(v);
}

template <typename T>
inline const void set_as(proto::historian::Type t,
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
  return as<MarketData>(IB_MARKET_DATA, record);
}

/** Returns as MarketDepth, if Record carries a MarketDepth. */
template <> inline optional<MarketDepth> as<MarketDepth>(const Record& record)
{
  return as<MarketDepth>(IB_MARKET_DEPTH, record);
}

/** Returns as SessionLog, if Record carries a SessionLog. */
template <> inline optional<SessionLog> as<SessionLog>(const Record& record)
{
  return as<SessionLog>(SESSION_LOG, record);
}

/** Returns as SessionLog, if Record carries a IndexedValue. */
template <> inline optional<IndexedValue> as<IndexedValue>(const Record& record)
{
  return as<IndexedValue>(INDEXED_VALUE, record);
}

template <typename T> inline void set_as(const T& v, Record* record)
{
  // do nothing
}

/** Sets the Record to carry a MarketData. */
template <> inline void set_as<MarketData>(const MarketData& v, Record* record)
{
  set_as<MarketData>(IB_MARKET_DATA, v, record);
}

/** Sets the Record to carry a MarketDepth. */
template <> inline void set_as<MarketDepth>(const MarketDepth& v,
                                            Record* record)
{
  set_as<MarketDepth>(IB_MARKET_DEPTH, v, record);
}

/** Sets the Record to carry a SessionLog. */
template <> inline void set_as<SessionLog>(const SessionLog& v, Record* record)
{
  set_as<SessionLog>(SESSION_LOG, v, record);
}

/** Sets the Record to carry a IndexedValue. */
template <> inline void set_as<IndexedValue>(const IndexedValue& v, Record* record)
{
  set_as<IndexedValue>(INDEXED_VALUE, v, record);
}

template <typename T> inline const Record wrap(const T& v)
{
  Record record;
  proto::historian::set_as<T>(v, &record);
  return record;
}

// template inline const Record wrap<MarketData>(const MarketData& v);
// template inline const Record wrap<MarketDepth>(const MarketDepth& v);
// template inline const Record wrap<SessionLog>(const SessionLog& v);
// template inline const Record wrap<IndexedValue>(const IndexedValue& v);


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

/** Wraps a IndexedValue in a copy of Record */
template <> inline const Record wrap<IndexedValue>(const IndexedValue& v)
{
  Record record; proto::historian::set_as(v, &record); return record;
}

} // historian
} // proto

#endif //PROTO_HISTORIAN_H_

