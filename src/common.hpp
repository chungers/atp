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


#endif // ATP_COMMON_H_
