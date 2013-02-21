#ifndef ATP_COMMON_TYPES_H_
#define ATP_COMMON_TYPES_H_

#include <boost/date_time/posix_time/posix_time.hpp>

#include "proto/ostream.hpp"

namespace atp {
namespace common {

typedef proto::platform::Id Id;
typedef boost::uint64_t microsecond_t;


} // common
} // atp

#endif //ATP_COMMON_TYPES_H_
