#ifndef ATP_COMMON_MOVING_WINDOW_H_
#define ATP_COMMON_MOVING_WINDOW_H_

#include <cmath>
#include <string>
#include <vector>

#include <boost/circular_buffer.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/pool/pool_alloc.hpp>
#include <boost/ref.hpp>
#include <boost/shared_ptr.hpp>

#include "log_levels.h"
#include "common/moving_window_callback.hpp"
#include "common/moving_window_interval_policy.hpp"
#include "common/moving_window_samplers.hpp"
#include "common/time_series.hpp"


using boost::function;
using boost::posix_time::time_duration;
using std::pair;
using std::string;
using std::vector;

using atp::common::sampler::latest;

namespace atp {
namespace common {

using namespace common::callback;

/// A moving window of time-based values
/// Different strategies for sampling can be set up.
template <
  typename element_t,
  typename sampler_t = function<element_t(const element_t& last,
                                          const element_t& current,
                                          bool new_sample_period) >,
  typename Alloc = boost::pool_allocator<element_t>,
  typename time_interval_policy = time_interval_policy::align_at_zero >
class moving_window : public time_series<microsecond_t, element_t>
{
 public:

  typedef typename
  time_series<microsecond_t, element_t>::series_operation
  series_operation;

  typedef typename
  time_series<microsecond_t, element_t>::sample_array_operation
  sample_array_operation;

  typedef typename
  time_series<microsecond_t, element_t>::value_array_operation
  value_array_operation;

  /// total duration, time resolution, and initial value
  moving_window(time_duration h, sample_interval_t i, element_t init) :
      history_duration_(h),
      interval_(i),
      buffer_(h.total_microseconds() / i.total_microseconds()),
      init_(init),
      current_value_(init),
      current_ts_(0),
      collected_(0),
      time_interval_policy_(i.total_microseconds()),
      post_process_(NULL)
  {
    // fill the buffer with the default valule.
    for (size_t i = 0; i < buffer_.capacity(); ++i) {
      buffer_.push_back(init);
    }
  }

  ~moving_window()
  {
  }

  /// Returns the duration of the history maintained by this moving_window
  const time_duration& history_duration() const
  {
    return history_duration_;
  }

  size_t total_events() const
  {
    return collected_;
  }

  size_t capacity() const
  {
    return buffer_.capacity() + 1;
  }

  /// Support for negative indexing as python
  /// -1 means the current value, -2 last element in buffer, ...
  virtual element_t operator[](int index) const
  {
    if (index == 0) {
      return current_value_;
    } else if (index < 0) {
      return buffer_[buffer_.size() + index];
    }
    return init_;
  }

  /// Returns the time period of samples, eg. 1 usec or 1 min
  virtual sample_interval_t time_period() const
  {
    return interval_;
  }

  virtual size_t size() const
  {
    return buffer_.size() + 1; // plus current unpushed value.
  }

  /// Must have negative index
  /// 0 means current observation for the current time interval
  /// -1 means the previous interval
  virtual microsecond_t get_time(int offset = 0) const
  {
    return time_interval_policy_.get_time(current_ts_, -offset);
  }

  /// Copies the last N values into the array
  template <typename buffer_t>
  size_t copy_last_data(buffer_t *array, const size_t length)
  {
    if (length > capacity()) {
      return 0; // No copy is done.
    }

    // current value is the last element:
    array[length - 1] = current_value_;

    typename history_t::array_range two = buffer_.array_two();
    typename history_t::array_range one = buffer_.array_one();

    size_t remain = length - 1;
    size_t to_copy2 = min(two.second, remain);
    size_t to_copy1 = remain - to_copy2;
    size_t sz = sizeof(buffer_t);
    size_t zero = 0;

    size_t dest_offset2 = remain - to_copy2;
    size_t src_offset2 = max(zero, two.second - to_copy2);
    buffer_t* dest_ptr2 = array + dest_offset2;
    buffer_t* src_ptr2 = two.first + src_offset2;

    size_t dest_offset1 = 0;
    size_t src_offset1 = max(zero, one.second - to_copy1);
    buffer_t* dest_ptr1 = array + dest_offset1;
    buffer_t* src_ptr1 = one.first + src_offset1;

    if (to_copy2 > 0) {
      memcpy(static_cast<void*>(dest_ptr2),
             static_cast<void*>(src_ptr2),
             to_copy2 * sz);
    }

    if (to_copy1 > 0) {
      memcpy(static_cast<void*>(dest_ptr1),
             static_cast<void*>(src_ptr1),
             to_copy1 * sz);
    }
    return to_copy2 + to_copy1 + 1;
  }

  /// Copies the last N samples (t, value) into the arrays
  template <typename buffer_t>
  size_t copy_last(microsecond_t *timestamp,
                   buffer_t *array,
                   const size_t length)
  {
    if (length > capacity()) {
      return 0; // No copy is done.
    }

    size_t copied1 = copy_last_data(array, length);
    size_t copied2 = generate_time_array(timestamp, length);
    assert(copied1 == copied2);
    return copied1;
  }

  /// Copies the last N samples (t, value) into the arrays
  /// This is the slow way, using reverse iterator
  template <typename buffer_t>
  size_t copy_last_slow(microsecond_t *timestamp,
                        buffer_t *array, const size_t length)
  {
    if (length > capacity()) {
      return 0; // No copy is done.
    }
    array[length - 1] = current_value_;
    timestamp[length - 1] = time_interval_policy_.get_time(current_ts_, 0);
    size_t to_copy = length - 1;
    size_t copied = 1;
    reverse_itr r = buffer_.rbegin();

    // fill the array in reverse order
    for (; r != buffer_.rend() && to_copy > 0; --to_copy, ++copied, ++r) {
      array[length - 1 - copied] = *r;
      timestamp[length - 1 - copied] =
          time_interval_policy_.get_time(current_ts_, copied);
    }
    return copied;
  }

  /// Returns the number of pushes into the time slots
  /// This allows the caller to track the number aggregated samples.
  size_t on(const microsecond_t& timestamp, const element_t& value)
  {
    return (*this)(timestamp, value);
  }

  size_t operator()(const microsecond_t& timestamp, const element_t& value)
  {
    int windows = time_interval_policy_.count_windows(current_ts_, timestamp);
    // Don't fill in missing values if starting up.
    if (current_ts_ > 0) {
      for (int i = 0; i < windows; ++i) {
        buffer_.push_back(current_value_);
        collected_++;
      }
    } else {
      windows = 0;
    }
    current_value_ = sampler_(current_value_, value,
                              windows > 0 || current_ts_ == 0);
    current_ts_ = timestamp;

    // Compute dependent indicators
    typename vector<series_operation_pair>::iterator itr;
    for (itr = series_operations.begin();
         itr != series_operations.end();
         ++itr) {
      element_t derived = itr->second.functor(*this);
      (*itr->second.series)(current_ts_, derived);
    }

    //////////////////////
    /// copy data and call the operators
    if (value_array_operations.size() > 0 ||
        sample_array_operations.size() > 0) {

      // element_t vbuff[len];  compiled ok on less restrictive compilers.
      const size_t len = capacity();
      element_t *vbuff = new element_t[len];
      size_t copied = copy_last_data(&vbuff[0], len);

      if (copied == len) {
        typename vector<value_array_operation_pair>::iterator itr;
        for (itr = value_array_operations.begin();
             itr != value_array_operations.end();
           ++itr) {
          element_t derived = itr->second.functor(&vbuff[0], len);
          (*itr->second.series)(current_ts_, derived);
        }
        if (sample_array_operations.size() > 0) {
          microsecond_t tbuff[len];
          size_t time_generated = generate_time_array(&tbuff[0], len);
          if (time_generated == len) {
            typename vector<sample_array_operation_pair>::iterator itr;
            for (itr = sample_array_operations.begin();
                 itr != sample_array_operations.end();
                 ++itr) {
              element_t derived = itr->second.functor(
                  &tbuff[0], &vbuff[0], len);
              (*itr->second.series)(current_ts_, derived);
            }
          } else {
            MOVING_WINDOW_ERROR << id()
                                << " cannot generate time array";
          }
        }
      } else {
        MOVING_WINDOW_ERROR << id()
                            << " cannot copy data array";
      }
      delete vbuff;
    }
    //////////////////////

    // Call the post process callback after time have advanced to the
    // next window
    size_t p = static_cast<size_t>(windows);
    if (post_process_ != NULL && p > 0) (*post_process_)(p, id(), *this);

    return p;
  }

  /// implements time_series::apply
  virtual time_series<microsecond_t, element_t>&
  apply(const string& id,
        series_operation op,
        const size_t min_samples = 1)
  {
    moving_window<element_t, latest<element_t> > *mw =
        new moving_window<element_t, latest<element_t> >(history_duration_,
                                                         interval_, init_);
    operation<series_operation> rec(id_, id,
                                    op, min_samples, mw);
    series_operations.push_back(series_operation_pair(id, rec));
    return *rec.series;
  }

  /// implements time_series::apply
  virtual time_series<microsecond_t, element_t>&
  apply2(const string& id,
         sample_array_operation op,
         const size_t min_samples = 1)
  {
    moving_window<element_t, latest<element_t> > *mw =
        new moving_window<element_t, latest<element_t> >(history_duration_,
                                                         interval_, init_);
    operation<sample_array_operation> rec(id_, id, op, min_samples, mw);
    sample_array_operations.push_back(sample_array_operation_pair(id, rec));
    return *rec.series;
  }

  /// implements time_series::apply
  virtual time_series<microsecond_t, element_t>&
  apply3(const string& id,
         value_array_operation op,
         const size_t min_samples = 1)
  {
    moving_window<element_t, latest<element_t> > *mw =
        new moving_window<element_t, latest<element_t> >(history_duration_,
                                                         interval_, init_);
    operation<value_array_operation> rec(id_, id, op, min_samples, mw);
    value_array_operations.push_back(value_array_operation_pair(id, rec));
    return *rec.series;
  }

  virtual time_series<microsecond_t, element_t>&
  apply3(const string& id,
         value_array_operation op,
         callback::moving_window_post_process<microsecond_t, element_t>& pp,
         const size_t min_samples = 1)
  {
    moving_window<element_t, latest<element_t> > *mw =
        new moving_window<element_t, latest<element_t> >(history_duration_,
                                                         interval_, init_);
    operation<value_array_operation> rec(id_, id, op, min_samples, mw);
    value_array_operations.push_back(value_array_operation_pair(id, rec));
    rec.series->set(pp);
    return *rec.series;
  }


  void set(const Id& id)
  {
    id_ = id;
  }

  virtual const Id& id() const
  {
    return id_;
  }

  void set(callback::moving_window_post_process<microsecond_t, element_t>& pp)
  {
    post_process_ = &pp;
  }


 private:

  /// Generate the time array based on the current time and the sampling
  /// intervals and the time interal policy.
  size_t generate_time_array(microsecond_t *timestamp, const size_t length)
  {
    if (length > capacity()) {
      return 0; // No copy is done.
    }
    timestamp[length - 1] = time_interval_policy_.get_time(current_ts_, 0);
    // fill the array in reverse order
    for (size_t to_copy = 0; to_copy < length - 1; ++to_copy) {
      timestamp[length - to_copy - 2] =
          time_interval_policy_.get_time(current_ts_, to_copy + 1);
    }
    return length;
  }


  typedef typename boost::circular_buffer<element_t, Alloc> history_t;
  typedef typename history_t::const_reverse_iterator reverse_itr;

  time_duration history_duration_;
  sample_interval_t interval_;
  history_t buffer_;

  element_t init_;
  element_t current_value_;
  microsecond_t current_ts_;
  size_t collected_;

  sampler_t sampler_;
  time_interval_policy time_interval_policy_;

  callback::moving_window_post_process<microsecond_t, element_t>* post_process_;

  template <typename operation_type>
  struct operation
  {
    explicit operation(const Id& parent_id,
                       const string id, operation_type op, size_t min_samples,
                       moving_window<element_t, latest<element_t> > *mw) :
        id(id), functor(op), min_samples(min_samples), series(mw)
    {
      Id sid = parent_id;
      sid.set_label(parent_id.label() + '$' + id);
      series->set(sid);
    }

    string id;
    operation_type functor;
    size_t min_samples;
    boost::shared_ptr< moving_window< element_t, latest<element_t> > > series;
  };

  typedef pair<string, operation<series_operation> >
  series_operation_pair;

  typedef pair<string, operation<sample_array_operation> >
  sample_array_operation_pair;

  typedef pair<string, operation<value_array_operation> >
  value_array_operation_pair;

  vector<series_operation_pair> series_operations;
  vector<sample_array_operation_pair> sample_array_operations;
  vector<value_array_operation_pair> value_array_operations;

  atp::common::Id id_;
};


template <typename element_t>
moving_window<element_t> join()
{

}



} // common
} // atp


#endif //ATP_COMMON_MOVING_WINDOW_H_
