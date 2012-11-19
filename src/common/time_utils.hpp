#ifndef ATP_TIME_UTILS_H_
#define ATP_TIME_UTILS_H_

#include <string>

#include <cctype>
#include <sys/time.h>
#include <boost/cstdint.hpp>
#include <boost/date_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/gregorian/greg_month.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <boost/date_time/local_time_adjustor.hpp>


using namespace boost::posix_time;
using boost::posix_time::ptime;
using boost::posix_time::time_duration;
using boost::uint64_t;

namespace atp {
namespace time {

//eastern timezone is utc-5
/// http://www.boost.org/doc/libs/1_37_0/doc/html/boost/date_time/local_adjustor.html
typedef boost::date_time::local_adjustor<ptime, -5, us_dst> us_eastern;
typedef boost::date_time::local_adjustor<ptime, -6, us_dst> us_central;
typedef boost::date_time::local_adjustor<ptime, -7, us_dst> us_mountain;
typedef boost::date_time::local_adjustor<ptime, -8, us_dst> us_pacific;


static const ptime EPOCH(boost::gregorian::date(1970,boost::gregorian::Jan,1));
static const std::locale TIME_FORMAT = std::locale(
    std::cout.getloc(),
    new boost::posix_time::time_input_facet("%Y-%m-%d %H:%M:%S%F%Q"));

static const time_duration RTH_START_EST(9, 30, 0, 0);
static const time_duration RTH_END_EST(16, 0, 0, 0);
static const time_duration EXT_START(4, 0, 0, 0);
static const time_duration EXT_END(20, 0, 0, 0);


/// from micros to posix time (ptime)
inline ptime as_ptime(const boost::uint64_t ts)
{
  ptime t = from_time_t(ts / 1000000LL);
  time_duration micros(0, 0, 0, ts % 1000000LL);
  t += micros;
  return t;
}

/// Microseconds from ptime.
inline boost::uint64_t as_micros(const ptime& pt)
{
  boost::uint64_t d = (pt.date() - EPOCH.date()).days()
      * 24 * 60 * 60 * 1000000LL;
  time_duration t = pt.time_of_day();
  boost::uint64_t m = d + t.total_microseconds();
  return m;
}

inline ptime to_est(const ptime& t)
{
  return us_eastern::utc_to_local(t);
}

/// Returns true if time given is within the regurlar trading hour (RTH)
inline bool checkRTH(const ptime& t)
{
  time_duration eastern = us_eastern::utc_to_local(t).time_of_day();
  return eastern >= RTH_START_EST && eastern < RTH_END_EST;
}

/// Returns true if time given is within the extended trading hours (EXT)
inline bool checkEXT(ptime t)
{
  time_duration eastern = us_eastern::utc_to_local(t).time_of_day();
  return eastern >= EXT_START && eastern < EXT_END;
}

/// Parse input string and set the ptime according to TIME_FORMAT
const inline bool parse(const std::string& input, ptime* output,
                        bool est=true)
{
  ptime pt;
  std::istringstream is(input);
  is.imbue(TIME_FORMAT);
  is >> pt;
  if (pt != ptime()) {
    *output = (!est) ? pt : us_eastern::local_to_utc(pt);
    return true;
  }
  return false;
}


} // namespace time
} // namespace atp

#endif //ATP_TIME_UTILS_H_
