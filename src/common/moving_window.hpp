#ifndef ATP_COMMON_MOVING_WINDOW_H_
#define ATP_COMMON_MOVING_WINDOW_H_


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


struct sample_interval_policy {


  struct align_at_zero
  {

    align_at_zero(const microsecond_t& window_size)
        : window_size_(window_size), mod_ts_(0)
    {
    }

    microsecond_t get_time(const microsecond_t& curr_ts,
                           const size_t& offset) const
    {
      microsecond_t r = curr_ts - (curr_ts % window_size_);
      return r - offset * window_size_;
    }

    int count_windows(const microsecond_t& last_ts,
                      const microsecond_t& timestamp)
    {
      return (timestamp - last_ts) / window_size_;
    }

    bool is_new_window(const microsecond_t& last_ts,
                       const microsecond_t& timestamp)
    {
      // track the remainder of the current timestamp.  if the current ts is
      // in the same time period, then the remainder will increase, until
      // it is outside the time period, where the remainder will be smaller
      // than the previous value.
      microsecond_t r = timestamp % window_size_;
      bool result =
          timestamp - last_ts > window_size_ ||
          timestamp > last_ts && r <= mod_ts_;
      mod_ts_ = r;
      return result;
    }

    microsecond_t window_size_;
    microsecond_t mod_ts_;
  };

};



template <
  typename element_t,
  typename sampler_t = boost::function<
    element_t(const element_t& last, const element_t& current,
              bool new_sample_period) >,
  typename Alloc = boost::pool_allocator<element_t>,
  typename sample_interval_policy = sample_interval_policy::align_at_zero >
class moving_window
{
 public:

  typedef boost::circular_buffer<element_t, Alloc> history_t;
  typedef typename
  boost::circular_buffer<element_t, Alloc>::const_reverse_iterator reverse_itr;

  /// total duration, time resolution, and initial value
  moving_window(boost::posix_time::time_duration h, sample_interval_t i,
                element_t init) :
      samples_(h.total_microseconds() / i.total_microseconds()),
      interval_(i),
      buffer_(samples_ - 1),
      current_value_(init),
      current_ts_(0),
      sample_interval_policy_(i.total_microseconds())
  {
    // fill the buffer with the default valule.
    for (size_t i = 0; i < buffer_.capacity(); ++i) {
      buffer_.push_back(init);
    }
  }

  /// Support for negative indexing as python
  /// -1 means the current value, -2 last element in buffer, ...
  const element_t operator[](int index) const
  {
    if (index >= 0) {
      return buffer_[index];
    } else {
      element_t val = current_value_;
      int start = index + 1;
      for (reverse_itr itr = buffer_.rbegin();
           start < 0; ++itr, ++start) {
        val = *itr;
      }
      return val;
    }
  }

  const long sample_microseconds() const
  {
    return interval_.total_microseconds();
  }

  const size_t capacity() const
  {
    return buffer_.capacity() + 1;
  }

  const size_t samples_buffered() const
  {
    return buffer_.size();
  }

  const size_t size() const
  {
    return buffer_.size() + 1; // plus current unpushed value.
  }

  template <typename buffer_t>
  const size_t copy_last(microsecond_t timestamp[],
                         buffer_t array[],
                         const size_t length) const
  {
    if (length > buffer_.capacity() + 1) {
      return 0; // No copy is done.
    }
    array[length - 1] = current_value_;
    timestamp[length - 1] = sample_interval_policy_.get_time(current_ts_, 0);
    size_t to_copy = length - 1;
    size_t copied = 1;
    reverse_itr r = buffer_.rbegin();

    // fill the array in reverse order
    for (; r != buffer_.rend() && to_copy > 0; --to_copy, ++copied, ++r) {
      array[length - 1 - copied] = *r;
      timestamp[length - 1 - copied] =
          sample_interval_policy_.get_time(current_ts_, copied);
    }
    return copied;
  }

  void on(microsecond_t timestamp, element_t value)
  {
    // check to see if the current timestamp falls within the current
    // interval.  If outside, push the current value onto the history buffer
    bool new_sample_period = sample_interval_policy_.is_new_window(
        current_ts_, timestamp);

    element_t sampled = sampler_(current_value_, value, new_sample_period);
    if (new_sample_period) {

      // check for missing values in previous windows
      if (current_ts_ > 0) {
        // Don't fill in missing values if starting up.
        int more = std::max(0, sample_interval_policy_.count_windows(
            current_ts_, timestamp) - 1);
        // TODO- allow filling in missing values like count
        element_t missing = current_value_;
        for (int i = 0; i < more; ++i) {
          buffer_.push_back(missing);
        }
      }

      // push the current value
      buffer_.push_back(current_value_);
    }
    current_value_ = sampled;
    current_ts_ = timestamp;
  }

 private:

  size_t samples_;

  sample_interval_t interval_;
  history_t buffer_;

  element_t current_value_;
  microsecond_t current_ts_;

  sampler_t sampler_;
  sample_interval_policy sample_interval_policy_;
};



} // time_series
} // atp


#endif //ATP_COMMON_MOVING_WINDOW_H_
