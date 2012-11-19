#ifndef ATP_TIME_SERIES_H_
#define ATP_TIME_SERIES_H_

#include <boost/assign.hpp>
#include <boost/bind.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/function.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/pool/pool_alloc.hpp>


namespace atp {
namespace time_series {


typedef boost::uint64_t microsecond_t;
typedef boost::posix_time::time_duration sample_interval_t;


} // time_series
} // atp


#endif //ATP_TIME_SERIES_H_
