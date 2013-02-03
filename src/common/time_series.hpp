#ifndef ATP_TIME_SERIES_H_
#define ATP_TIME_SERIES_H_

#include <boost/function.hpp>


namespace atp {
namespace time_series {


typedef boost::uint64_t microsecond_t;
typedef boost::posix_time::time_duration sample_interval_t;

template <typename T, typename V> class data_series;


  /// Syntax sugar -- a class that has the array element operator
  /// to allow specification of time using foo.t[-1] syntax.
template <typename T, typename V>
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


template <typename T, typename V>
class data_series
{

 public:

  typedef boost::function< V(const data_series<T, V>&) > series_operation;
  typedef boost::function< V(T*, V*, size_t) > sample_array_operation;
  typedef boost::function< V(V*, size_t) > value_array_operation;

  data_series() : t(*this) {}

  /// supports backwards indexing (negative index)
  virtual V operator[](int index) const = 0;
  virtual T get_time(int index) const = 0;
  virtual size_t size() const = 0;
  virtual sample_interval_t time_period() const = 0;

  virtual data_series<T, V>& apply(const std::string& id,
                                   series_operation op,
                                   const size_t min_samples = 1) = 0;

  virtual data_series<T, V>& apply2(const std::string& id,
                                   sample_array_operation op,
                                   const size_t min_samples = 1) = 0;

  virtual data_series<T, V>& apply3(const std::string& id,
                                    value_array_operation op,
                                    const size_t min_samples = 1) = 0;

  const time_axis<T, V> t;
};




} // time_series
} // atp


#endif //ATP_TIME_SERIES_H_
