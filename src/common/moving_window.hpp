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


template <typename element_t>
struct sampler {

  struct latest
  {
    const element_t operator()(const element_t& last,
                                const element_t& now,
                                bool new_period)
    {
      return now;
    }
  };

  class avg
  {
   public:
    avg() : count(0) {}

    const element_t operator()(const element_t& last,
                                const element_t& now,
                                bool new_period)
    {
      if (new_period) {
        count = 0;
        sum = now;
      }
      sum = (count == 0) ? sum = now : sum + now;
      return sum / static_cast<element_t>(++count);
    }

   private:
    size_t count;
    element_t sum;
  };

  struct max
  {
    const element_t operator()(const element_t& last,
                                const element_t& now,
                                bool new_period)
    {
      return (new_period) ? now : std::max(last, now);
    }
  };

  struct min
  {
    const element_t operator()(const element_t& last,
                                const element_t& now,
                                bool new_period)
    {
      return (new_period) ? now : std::min(last, now);
    }
  };

  class open
  {
   public:

    open() : init_(false) {}

    const element_t operator()(const element_t& last,
                                const element_t& now,
                                bool new_period)
    {
      if (new_period) { open_ = now; init_ = true; }
      if (!init_) { open_ = now; init_ = true; }
      return open_;
    }

   private:
    element_t open_;
    bool init_;
  };

  struct close
  {
    const element_t operator()(const element_t& last,
                                const element_t& now,
                                bool new_period)
    {
      return now;
    }
  };

}; // sampler


struct sample_interval_policy {


  struct align_at_zero
  {

    align_at_zero(const microsecond_t& window_size)
        : window_size_(window_size)
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
      if (timestamp < last_ts) return 0;

      microsecond_t m1 = last_ts - (last_ts % window_size_);
      microsecond_t m2 = timestamp - (timestamp % window_size_);
      return (m2 - m1) / window_size_;
    }

    bool is_new_window(const microsecond_t& last_ts,
                       const microsecond_t& timestamp)
    {
      return count_windows(last_ts, timestamp) > 0;
    }

    microsecond_t window_size_;
  };

};



/// A moving window of time-based values
/// Different strategies for sampling can be set up.
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
    int windows = sample_interval_policy_.count_windows(current_ts_, timestamp);
    // Don't fill in missing values if starting up.
    if (current_ts_ > 0) {
      for (int i = 0; i < windows; ++i) {
        buffer_.push_back(current_value_);
      }
    }
    current_value_ = sampler_(current_value_, value, windows > 0);
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
