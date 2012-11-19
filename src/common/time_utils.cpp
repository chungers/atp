
#include "common/time_utils.hpp"

using namespace boost::posix_time;
using boost::posix_time::ptime;
using boost::posix_time::time_duration;
using boost::uint64_t;

namespace atp {
namespace time {

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
                        bool est)
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
