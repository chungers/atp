#ifndef ATP_PLATFORM_CONTROL_MESSAGE_HANDLER_IMPL_H_
#define ATP_PLATFORM_CONTROL_MESSAGE_HANDLER_IMPL_H_

#include <string>

#include "platform/generic_handler.hpp"
#include "proto/platform.pb.h"


/// Contains template specializations for proto::platform::ControlMessage

using std::string;
using atp::platform::types::timestamp_t;
using proto::platform::ControlMessage;

namespace atp {
namespace platform {


template <>
inline bool deserialize(const string& raw, ControlMessage& m)
{
  return m.ParseFromString(raw);
}

template <>
inline timestamp_t get_timestamp<ControlMessage>(const ControlMessage& m)
{
  return m.timestamp();
}

template <>
inline string get_message_key(const ControlMessage& m)
{
  return m.GetTypeName();
}


} // platform
} // atp


#endif //ATP_PLATFORM_CONTROL_MESSAGE_HANDLER_IMPL_H_
