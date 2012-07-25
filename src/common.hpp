#ifndef ATP_COMMON_H_
#define ATP_COMMON_H_

#include <iostream>
#include <string>
#include <stdarg.h>

#include <boost/scoped_ptr.hpp>
#include <boost/utility.hpp>

#include "constants.h"
#include "log_levels.h"
#include "utils.hpp"

typedef boost::noncopyable NoCopyAndAssign;


// Whether we should die when reporting an error.
enum DieWhenReporting { DIE, DO_NOT_DIE };

// Report Error and exit if requested.
inline static void ReportError(DieWhenReporting should_die,
                               const char* format, ...)
{
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
  TimeTracking() : currentMicros_(0),
                   facet_("%Y-%m-%d %H:%M:%S%F%Q")
  {
  }

  ~TimeTracking() {}

  void getISOTime(std::string* output)
  {
    std::ostringstream ss;
    formatTime(&ss);
    ss << currentMicros_;
    output->assign(ss.str());
  }

  void formatTime(std::ostream* outstream)
  {
    // dynamically allocate the locale since stack allocated locale will
    // crash the program in the locale's ~Impl()
    std::locale* l = new std::locale(std::cout.getloc(), &facet_);
    outstream->imbue(*l);
  }

  inline boost::uint64_t getMicros()
  {
    return currentMicros_;
  }

 protected:
  inline boost::uint64_t setMicros(boost::uint64_t t)
  {
    currentMicros_ = t;
    return currentMicros_;
  }

  inline boost::uint64_t now()
  {
    currentMicros_ = now_micros();
    return currentMicros_;
  }

 private:
  boost::uint64_t currentMicros_;
  boost::posix_time::time_facet facet_;
};
}
}

#endif // ATP_COMMON_H_
