#ifndef ATP_TIME_SERIES_TIME_SERIES_H_
#define ATP_TIME_SERIES_TIME_SERIES_H_

#include <boost/function.hpp>

#include "common/types.hpp"
#include "time_series/time_utils.hpp"


using atp::common::microsecond_t;


namespace atp {
namespace time_series {

typedef boost::posix_time::time_duration sample_interval_t;


template <typename T, typename V>
class time_series
{

 public:

  typedef boost::function< V(const time_series<T, V>&) >
  series_operation;

  typedef boost::function< V(const T*, const V*, const size_t) >
  sample_array_operation;

  typedef boost::function< V(const microsecond_t& current_t,
                             const V*, const size_t) >
  value_array_operation;

  virtual ~time_series()
  {
  }

  /// supports backwards indexing (negative index)
  virtual V operator[](int index) const = 0;
  virtual T get_time(int index) const = 0;
  virtual size_t size() const = 0;
  virtual sample_interval_t time_period() const = 0;
  virtual const Id& id() const = 0;

  /// Three different forms of apply with different functor types
  /// Note that we have to use different function names because
  /// a class can overload all three operators and so we need different
  /// function names to disambiguate which corresponding functor we want to
  /// use.
  virtual time_series<T, V>& apply(const std::string& id,
                                   series_operation op,
                                   const size_t min_samples = 1) = 0;

  virtual time_series<T, V>& apply2(const std::string& id,
                                    sample_array_operation op,
                                    const size_t min_samples = 1) = 0;

  virtual time_series<T, V>& apply3(const std::string& id,
                                    value_array_operation op,
                                    const size_t min_samples = 1) = 0;
};




} // time_series
} // atp


#endif //ATP_TIME_SERIES_TIME_SERIES_H_
