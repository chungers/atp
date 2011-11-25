#ifndef ATP_COMMON_H_
#define ATP_COMMON_H_

#include <iostream>
#include <string>

#include <boost/scoped_ptr.hpp>
#include <boost/utility.hpp>

#include "constants.h"
#include "log_levels.h"
#include "utils.hpp"


typedef boost::noncopyable NoCopyAndAssign;


// Whether we should die when reporting an error.
enum DieWhenReporting { DIE, DO_NOT_DIE };

// Report Error and exit if requested.
static void ReportError(DieWhenReporting should_die, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);
  if (should_die == DIE)
    exit(1);   // almost certainly exit()
}

namespace ib {
namespace internal {

class TimeTracking
{
 public:
  TimeTracking() : currentUtcMicros_(0), currentMicros_(0),
                   facet_("%Y-%m-%d %H:%M:%S%F%Q")
  {
  }

  ~TimeTracking() {}

  void getISOTime(std::string* output)
  {
    std::ostringstream ss;
    formatTime(&ss);
    ss << currentUtcMicros_;
    output->assign(ss.str());
  }

  void formatTime(std::ostream* outstream)
  {
    outstream->imbue(std::locale(std::cout.getloc(), &facet_));
  }

  boost::uint64_t getMicros()
  {
    return currentMicros_;
  }

  boost::uint64_t getUtcMicros()
  {
    return currentUtcMicros_;
  }

 protected:
  boost::uint64_t setUtcMicros(boost::uint64_t utc)
  {
    return currentUtcMicros_ = utc;
  }

  boost::uint64_t setMicros(boost::uint64_t t)
  {
    return currentMicros_ = t;
  }
 private:
  boost::uint64_t currentUtcMicros_;
  boost::uint64_t currentMicros_;
  boost::posix_time::time_facet facet_;
};
} // namespace internal
} // namespace ib

#endif // ATP_COMMON_H_
