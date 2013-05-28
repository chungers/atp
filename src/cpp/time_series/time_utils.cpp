
#include "time_series/time_utils.hpp"

using namespace boost::posix_time;
using boost::posix_time::ptime;
using boost::posix_time::time_duration;
using boost::uint64_t;

namespace atp {
namespace time_series {

/// Returns true if time given is within the regurlar trading hour (RTH)
bool checkRTH(const ptime& t)
{
  time_duration eastern = us_eastern::utc_to_local(t).time_of_day();
  return eastern >= RTH_START_EST && eastern < RTH_END_EST;
}

/// Returns true if time given is within the extended trading hours (EXT)
bool checkEXT(ptime t)
{
  time_duration eastern = us_eastern::utc_to_local(t).time_of_day();
  return eastern >= EXT_START && eastern < EXT_END;
}

/// Parse input string and set the ptime according to TIME_FORMAT
bool parse(const std::string& input, ptime* output, bool est)
{
  ptime pt(time_from_string(input));
  if (pt != ptime()) {
    *output = (!est) ? pt : us_eastern::local_to_utc(pt);
    return true;
  }
  return false;
}

} // namespace time_series
} // namespace atp
