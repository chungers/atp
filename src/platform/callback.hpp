#ifndef ATP_PLATFORM_CALLBACK_H_
#define ATP_PLATFORM_CALLBACK_H_

#include <boost/function.hpp>

#include "platform/types.hpp"

using atp::platform::types::timestamp_t;
using boost::function;

namespace atp {
namespace platform {
namespace callback {

template <typename V>
struct update_event
{
  typedef function<void(const timestamp_t&, const V&) > func;
};

} // callback
} // platform
} // atp

#endif //ATP_PLATFORM_CALLBACK_H_
