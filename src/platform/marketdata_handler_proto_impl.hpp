#ifndef ATP_PLATFORM_MARKETDATA_HANDLER_PROTO_IMPL_H_
#define ATP_PLATFORM_MARKETDATA_HANDLER_PROTO_IMPL_H_

#include "platform/marketdata_handler.hpp"

#include "proto/common.pb.h"
#include "proto/ib.pb.h"


/// Contains template specializations for proto::ib::MarketData

using namespace atp::platform::marketdata;
using atp::platform::types::timestamp_t;
using proto::common::Value;
using proto::ib::MarketData;

namespace atp {
namespace platform {
namespace marketdata {

template <>
inline bool deserialize(const string& raw, MarketData& m)
{
  return m.ParseFromString(raw);
}

template <>
inline const timestamp_t get_timestamp<MarketData>(const MarketData& m)
{
  return m.timestamp();
}

template <>
inline const string& get_event_code<MarketData, string>(const MarketData& m)
{
  return m.event();
}

template <>
int value_updater<MarketData, string>::operator()(const timestamp_t& ts,
                                                  const string& event_code,
                                                  const MarketData& event)
{
  switch (event.value().type()) {
    case proto::common::Value::DOUBLE: {
      return double_dispatcher_.dispatch(
          event_code, ts, event.value().double_value());
    }

    case proto::common::Value::INT: {
      return int_dispatcher_.dispatch(
          event_code, ts, event.value().int_value());
    }

    case proto::common::Value::STRING: {
      return string_dispatcher_.dispatch(
          event_code, ts, event.value().string_value());
    }
    default:
      return atp::platform::marketdata::error_code::NO_VALUE_TYPE_MATCH;
  }
}

} // marketdata
} // platform
} // atp


#endif //ATP_PLATFORM_MARKETDATA_HANDLER_PROTO_IMPL_H_
