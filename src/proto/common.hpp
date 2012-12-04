#ifndef PROTO_COMMON_H_
#define PROTO_COMMON_H_

#include <iostream>
#include <string>
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>

#include "proto/ib.pb.h"

// Utilites

namespace proto {
namespace common {

using std::string;
using boost::optional;

namespace internal {

template <typename T>
inline optional<T> get(const Value& value)
{
  return optional<T>();
}

template <>
inline optional<double> get<double>(const Value& value)
{
  return optional<double>(value.double_value());
}

template <>
inline optional<int> get<int>(const Value& value)
{
  return optional<int>(value.int_value());
}

template <>
inline optional<string> get<string>(const Value& value)
{
  return optional<string>(value.string_value());
}

template <typename T>
inline optional<T> as(const proto::common::Value_Type type, const Value& value)
{
  if (value.type() == type) {
    return get<T>(value);
  } else {
    return optional<T>(); // uninitialized.
  }
}

inline void set_as(const double& v, Value* val)
{
  val->set_double_value(v);
}

inline void set_as(const int& v, Value* val)
{
  val->set_int_value(v);
}

inline void set_as(const string& v, Value* val)
{
  val->set_string_value(v);
}

template <typename T>
inline void set_as(proto::common::Value_Type t, const T& v, Value* val)
{
  val->set_type(t);
  set_as(v, val);
}


} // internal

using namespace internal;

/**
   Returns the typed value contained in the Value proto.
   This uses boost::optional to handle the case where the Value is
   carrying an int but the caller tries to get the value as something other
   than an int.  In that case if(optional<T>) will be false.
   Use the *(optional) to access the value itself.
*/
template <typename T> inline optional<T> as(const Value& value)
{
  return optional<T>();
}

/** Returns as double, if Value carries a double_value. */
template <> inline optional<double> as(const Value& value)
{
  return as<double>(proto::common::Value_Type_DOUBLE, value);
}

/** Returns as int, if Value carries a int_value. */
template <> inline optional<int> as<int>(const Value& value)
{
  return as<int>(proto::common::Value_Type_INT, value);
}

/** Returns as string, if Value carries a string_value. */
template <> inline optional<string> as<string>(const Value& value)
{
  return as<string>(proto::common::Value_Type_STRING, value);
}

template <typename T> inline void set_as(const T& v, Value* val)
{
  set_as<string>(proto::common::Value_Type_STRING,
                 boost::lexical_cast<string>(v), val);
}

/** Sets the Value to carry a double. */
template <> inline void set_as<double>(const double& v, Value* val)
{
  set_as<double>(proto::common::Value_Type_DOUBLE, v, val);
}

/** Sets the Value to carry a int. */
template <> inline void set_as<int>(const int& v, Value* val)
{
  set_as<int>(proto::common::Value_Type_INT, v, val);
}

/** Sets the Value to carry a string. */
template <> inline void set_as<string>(const string& v, Value* val)
{
  set_as<string>(proto::common::Value_Type_STRING, v, val);
}

template <typename T> inline const Value wrap(const T& v)
{
  // Default try to carry every other type as a string
  Value val;
  proto::common::set_as(boost::lexical_cast<string>(v), &val);
  return val;
}

/** Wraps a double in a copy of Value */
template <> inline const Value wrap<double>(const double& v)
{
  Value val; proto::common::set_as(v, &val); return val;
}

/** Wraps a int in a copy of Value */
template <> inline const Value wrap<int>(const int& v)
{
  Value val; proto::common::set_as(v, &val); return val;
}

/** Wraps a string in a copy of Value */
template <> inline const Value wrap<string>(const string& v)
{
  Value val; proto::common::set_as(v, &val); return val;
}

} // common
} // proto

#endif //PROTO_COMMON_H_

