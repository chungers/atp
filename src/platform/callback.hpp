#ifndef ATP_PLATFORM_CALLBACK_H_
#define ATP_PLATFORM_CALLBACK_H_

#include <boost/function.hpp>

#include "platform/types.hpp"

using atp::platform::types::timestamp_t;
using boost::function;

namespace atp {
namespace platform {
namespace callback {

typedef function<void(const timestamp_t&, const double&) > double_updater;
typedef function<void(const timestamp_t&, const int&) > int_updater;
typedef function<void(const timestamp_t&, const string&) > string_updater;

} // callback
} // platform
} // atp

#endif //ATP_PLATFORM_CALLBACK_H_
