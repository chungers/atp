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


template <typename T, typename V>
class data_series
{
  /// Syntax sugar -- a class that has the array element operator
  /// to allow specification of time using foo.t[-1] syntax.
  class time_axis {
   public:
    time_axis(const data_series<T, V>& series) : series(series) {};
    const T operator[](int i) const
    {
      return series.get_time(i);
    }
   private:
    const data_series<T,V>& series;
  };

 public:

  data_series() : t(*this) {}

  /// supports backwards indexing (negative index)
  virtual V operator[](int index) const = 0;
  virtual T get_time(int index) const = 0;
  virtual size_t size() const = 0;
  virtual sample_interval_t time_period() const = 0;

  const time_axis t;
};




} // time_series
} // atp


#endif //ATP_TIME_SERIES_H_
