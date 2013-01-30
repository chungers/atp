

#include <cmath>
#include <string>

#include <boost/circular_buffer.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/pool/pool_alloc.hpp>
#include <boost/pool/singleton_pool.hpp>

#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include <glog/logging.h>

#include <ta-lib/ta_func.h>
#include "utils.hpp"

#include "test.pb.h"


DEFINE_int32(num_objects_to_create, 1000, "Number of objects to create.");

using boost::circular_buffer;
typedef boost::uint64_t microsecond_t;

TEST(MiscPrototype, CircularBuffer1)
{
  circular_buffer<int> cb(5);
  cb.push_back(1);
  cb.push_back(2);
  cb.push_back(3);
  cb.push_back(4);
  cb.push_back(5);

  EXPECT_EQ(1, cb[0]);
  EXPECT_EQ(2, cb[1]);
  EXPECT_EQ(3, cb[2]);
  EXPECT_EQ(4, cb[3]);
  EXPECT_EQ(5, cb[4]);

  cb.push_back(6);

  EXPECT_EQ(2, cb[0]);
  EXPECT_EQ(3, cb[1]);
  EXPECT_EQ(4, cb[2]);
  EXPECT_EQ(5, cb[3]);
  EXPECT_EQ(6, cb[4]);

  EXPECT_EQ(5, cb.size());
  EXPECT_EQ(5, cb.capacity());

  EXPECT_EQ(6, *cb.rbegin());

  cb.push_back(7);

  typename boost::circular_buffer<int>::array_range one = cb.array_one();
  typename boost::circular_buffer<int>::array_range two = cb.array_two();
  LOG(INFO) << "one = " << one.first << ", " << one.second;
  LOG(INFO) << "two = " << two.first << ", " << two.second;

  // copy the last 3 elements -- in reverse order
  size_t length = 3;
  size_t sz = sizeof(int);
  int buff[length];

  void* array = static_cast<void*>(&buff[0]);
  memset(array, 0, sz * length);
  for (int i = 0; i < length; ++i) {
    LOG(INFO) << "set buff[" << i << "] = " << buff[i];
  }

  size_t to_copy2 = min(two.second, length);
  size_t to_copy1 = length - to_copy2;

  size_t dest_offset2 = length - to_copy2;
  size_t src_offset2 = max(static_cast<size_t>(0), two.second - to_copy2);
  void* dest_ptr2 = array + dest_offset2 * sz;
  void* src_ptr2 = static_cast<void*>(two.first) + src_offset2 * sz;
  size_t bytes2 = to_copy2 * sz;

  if (to_copy2 > 0) {
    memcpy(dest_ptr2, src_ptr2, bytes2);
  }

  size_t dest_offset1 = 0;
  size_t src_offset1 = max(static_cast<size_t>(0), one.second - to_copy1);
  void* dest_ptr1 = array + dest_offset1 * sz;
  void* src_ptr1 = static_cast<void*>(one.first) + src_offset1 * sz;
  size_t bytes1 = to_copy1 * sz;

  if (to_copy1 > 0) {
    memcpy(dest_ptr1, src_ptr1, bytes1);
  }

  for (int i = 0; i < length; ++i) {
    LOG(INFO) << "buff[" << i << "] = " << buff[i];
  }
}

/// fill the array in reverse order
template <typename buffer_t>
size_t copy_last(circular_buffer<buffer_t>& cb,
                 buffer_t *array,
                 const size_t length)
{
  size_t to_copy = length;
  size_t copied = 0;
  typename circular_buffer<buffer_t>::reverse_iterator r = cb.rbegin();
  for (; r != cb.rend() && to_copy > 0; --to_copy, ++copied, ++r) {
    array[length - 1 - copied] = *r;
  }
  return copied;
}

/// fill the array in reverse order -- calls linearize
template <typename buffer_t>
size_t copy_last2(circular_buffer<buffer_t>& cb,
                  buffer_t *array,
                  const size_t length)
{
  size_t to_copy = length;
  size_t copied = 0;
  cb.linearize();
  typename circular_buffer<buffer_t>::reverse_iterator r = cb.rbegin();
  for (; r != cb.rend() && to_copy > 0; --to_copy, ++copied, ++r) {
    array[length - 1 - copied] = *r;
  }
  return copied;
}

/// fill the array in reverse order -- use array_one and array_two
template <typename buffer_t>
size_t copy_last3(circular_buffer<buffer_t>& cb,
                  buffer_t *array,
                  const size_t length)
{
  typename boost::circular_buffer<buffer_t>::array_range two = cb.array_two();
  typename boost::circular_buffer<buffer_t>::array_range one = cb.array_one();

  buffer_t* array_ptr = array;
  size_t array_len = length;

  size_t to_copy2 = min(two.second, length);
  size_t to_copy1 = length - to_copy2;
  size_t sz = sizeof(buffer_t);
  size_t zero = 0;

  // LOG(INFO) << "two.sz = " << two.second
  //           << ", one.sz = " << one.second
  //           << ", to_copy2 = " << to_copy2
  //           << ", to_copy1 = " << to_copy1
  //           << ", sz = " << sz;

  size_t dest_offset2 = length - to_copy2;
  size_t src_offset2 = max(zero, two.second - to_copy2);
  void* dest_ptr2 = static_cast<void*>(array) + dest_offset2 * sz;
  void* src_ptr2 = static_cast<void*>(two.first) + src_offset2 * sz;
  size_t bytes2 = to_copy2 * sz;

  size_t dest_offset1 = 0;
  size_t src_offset1 = max(zero, one.second - to_copy1);
  void* dest_ptr1 = static_cast<void*>(array) + dest_offset1 * sz;
  void* src_ptr1 = static_cast<void*>(one.first) + src_offset1 * sz;
  size_t bytes1 = to_copy1 * sz;

  if (to_copy2 > 0) {
    memcpy(dest_ptr2, src_ptr2, bytes2);
  }

  if (to_copy1 > 0) {
    memcpy(dest_ptr1, src_ptr1, bytes1);
  }
  return to_copy2 + to_copy1;
}

TEST(MiscPrototype, CircularBufferArrayCopy)
{
  // for indicators we tend to use small numbrer of points of history
  // eg. SMA(20), etc. but executes many many times
  int events = 100000;

  size_t len = 100;
  circular_buffer<double> cb(len);

  microsecond_t total_copy = 0;
  microsecond_t start = now_micros();
  double buff[len / 2];
  for (int i = 0; i < events; ++i) {
    cb.push_back(static_cast<double>(i) * 2.);

    if (cb.size() >= len) {
      microsecond_t copy_start = now_micros();
      EXPECT_EQ(len / 2, copy_last(cb, &buff[0], len/2));
      total_copy += now_micros() - copy_start;
    }
  }
  microsecond_t dt = now_micros() - start;

  LOG(INFO) << "avg dt (reverse iterator) = " << total_copy / events;

  ////////////////////////////
  /// Alternative implementation of copy
  total_copy = 0;
  start = now_micros();
  for (int i = 0; i < events; ++i) {
    cb.push_back(static_cast<double>(i) * 2.);

    if (cb.size() >= len) {
      microsecond_t copy_start = now_micros();
      EXPECT_EQ(len / 2, copy_last2(cb, &buff[0], len/2));
      total_copy += now_micros() - copy_start;
    }
  }
  dt = now_micros() - start;
  LOG(INFO) << "avg dt (reverse iterator+linearize) = " << total_copy / events;

  ////////////////////////////
  /// Alternative implementation of copy
  total_copy = 0;
  start = now_micros();
  for (int i = 0; i < events; ++i) {
    cb.push_back(static_cast<double>(i) * 2.);

    if (cb.size() >= len) {
      microsecond_t copy_start = now_micros();
      EXPECT_EQ(len / 2, copy_last3<double>(cb, &buff[0], len/2));
      total_copy += now_micros() - copy_start;
    }
  }
  dt = now_micros() - start;
  LOG(INFO) << "avg dt (memcpy + array_one/two) = " << total_copy / events;
}

TEST(MiscPrototype, CircularBuffer)
{
  using proto::test::Candle;

  boost::uint64_t now = now_micros();

  size_t length = 100;
  boost::circular_buffer<Candle> cb(length);
  for (int i = 0 ; i < length ; ++i) {
    Candle c;
    c.set_timestamp(i);
    cb.push_back(c);
  }

  for (int i = 0 ; i < length ; ++i) {
    EXPECT_EQ(i, cb[i].timestamp());
  }

  for (int i = length ; i < 2*length ; ++i) {
    Candle c;
    c.set_timestamp(i);
    cb.push_back(c);
  }

  for (int i = 0 ; i < length ; ++i) {
    EXPECT_EQ(length + i, cb[i].timestamp());
  }

  boost::uint64_t elapsed = now_micros() - now;
  LOG(INFO) << length << " objects in " << elapsed << " usecs";
}

TEST(MiscPrototype, CircularBufferTiming)
{
  using proto::test::Candle;
  typedef boost::circular_buffer<Candle> ring;
  typedef boost::circular_buffer<Candle>::const_iterator ring_itr;

  size_t length = 100;
  ring cb(length);
  for (int i = 0 ; i < length ; ++i) {
    Candle c;
    c.set_timestamp(i);
    cb.push_back(c);
  }

  // time the case where we have one data point added to the
  // buffer and then iterate through the entire ring
  boost::uint64_t now = now_micros();
  Candle c;
  c.set_timestamp(length);
  cb.push_back(c);

  int i = 0;
  double sum = 0;
  for (ring_itr itr = cb.begin(); itr != cb.end(); ++itr, ++i) {
    EXPECT_EQ(1 + i, itr->timestamp());
    sum += itr->timestamp();
  }
  double avg = sum / length;

  boost::uint64_t elapsed = now_micros() - now;
  LOG(INFO) << "Avg = " << avg << " over "
            << length << " objects in " << elapsed << " usecs";
}

struct CandleTag {};
TEST(MiscPrototype, ObjectPool1)
{
  using proto::test::Candle;
  typedef boost::singleton_pool<CandleTag, sizeof(Candle)> singleton_pool;

  typedef boost::object_pool<Candle> pool;
  typedef boost::uint64_t ts;

  size_t num_objects = FLAGS_num_objects_to_create;
  ts start, elapsed;

  //////////////////////////////
  {
    start = now_micros();

    for (int i = 0; i < num_objects; ++i) {
      void *c = singleton_pool::malloc();
      singleton_pool::free(c); // no destructor call
    }

    elapsed = now_micros() - start;
    LOG(INFO) << "singleton_pool " << num_objects << " objects in "
              << elapsed << " usecs (with free)";
  }

  //////////////////////////////
  {
    start = now_micros();

    pool p;
    for (int i = 0; i < num_objects; ++i) {
      Candle *c = p.construct();
      p.free(c); // deallocates memory but does not call destructor
    }

    elapsed = now_micros() - start;
    LOG(INFO) << "pool.construct " << num_objects << " objects in "
              << elapsed << " usecs (with free)";
  }
  //////////////////////////////
  {
    start = now_micros();

    pool p;
    for (int i = 0; i < num_objects; ++i) {
      Candle *c = p.construct();
      p.destroy(c); // deallocates memory and calls destructor
    }

    elapsed = now_micros() - start;
    LOG(INFO) << "pool.construct " << num_objects << " objects in "
              << elapsed << " usecs (with destroy)";
  }

  //////////////////////////////
  {
    start = now_micros();

    pool p;
    for (int i = 0; i < num_objects; ++i) {
      Candle *c = p.construct();
    }

    elapsed = now_micros() - start;
    LOG(INFO) << "pool.construct " << num_objects << " objects in "
              << elapsed << " usecs (no free)";
  }

  //////////////////////////////
  {
    start = now_micros();
    for (int i = 0; i < num_objects; ++i) {
      Candle *c = new Candle();
      delete c;
    }

    elapsed = now_micros() - start;
    LOG(INFO) << "    new/delete " << num_objects << " objects in "
              << elapsed << " usecs";
  }

  //////////////////////////////
  {
    start = now_micros();
    for (int i = 0; i < num_objects; ++i) {
      Candle c;
    }

    elapsed = now_micros() - start;
    LOG(INFO) << "   stack alloc " << num_objects << " objects in "
              << elapsed << " usecs";
  }
}

TEST(MiscPrototype, CircularBufferWithPoolAllocator)
{
  using proto::test::Candle;
  typedef boost::object_pool<Candle> pool;
  typedef boost::circular_buffer<Candle> ring;
  typedef boost::circular_buffer<
    Candle, boost::pool_allocator<Candle> > pooled_ring;
  typedef boost::circular_buffer<
    Candle, boost::fast_pool_allocator<Candle> > fast_pooled_ring;
  typedef boost::circular_buffer<Candle> unpooled_ring;
  typedef boost::circular_buffer<Candle>::const_iterator ring_itr;
  typedef boost::uint64_t ts;

  ts start, elapsed;
  size_t num_objects = FLAGS_num_objects_to_create;
  size_t length = 100;

  ////////////////////////////////
  {
    start = now_micros();
    ring cb(length);
    for (int i = 0 ; i < num_objects ; ++i) {
      Candle c;
      c.set_timestamp(i);
      cb.push_back(c);
    }

    elapsed = now_micros() - start;
    LOG(INFO) << "     stack alloc + ring: " << num_objects << " objects in "
              << elapsed << " usecs";
  }

  ////////////////////////////////
  {
    start = now_micros();
    pooled_ring cb(length);
    for (int i = 0 ; i < num_objects ; ++i) {
      Candle c;
      c.set_timestamp(i);
      cb.push_back(c);
    }

    elapsed = now_micros() - start;
    LOG(INFO) << "            pooled_ring: " << num_objects << " objects in "
              << elapsed << " usecs";
  }

  ////////////////////////////////
  {
    start = now_micros();
    fast_pooled_ring cb(length);
    for (int i = 0 ; i < num_objects ; ++i) {
      Candle c;
      c.set_timestamp(i);
      cb.push_back(c);
    }

    elapsed = now_micros() - start;
    LOG(INFO) << "       fast_pooled_ring: " << num_objects << " objects in "
              << elapsed << " usecs";
  }

  ////////////////////////////////
  {
    start = now_micros();
    unpooled_ring cb(length);
    for (int i = 0 ; i < num_objects ; ++i) {
      Candle c;
      c.set_timestamp(i);
      cb.push_back(c);
    }

    elapsed = now_micros() - start;
    LOG(INFO) << "          unpooled_ring: " << num_objects << " objects in "
              << elapsed << " usecs";
  }

  ////////////////////////////////
  {
    start = now_micros();
    pool p;
    pooled_ring cb(length);
    for (int i = 0 ; i < num_objects ; ++i) {
      Candle* c = p.construct();
      c->set_timestamp(i);
      cb.push_back(*c);
      p.free(c);
    }

    elapsed = now_micros() - start;
    LOG(INFO) << "     pool + pooled_ring: " << num_objects << " objects in "
              << elapsed << " usecs";
  }

  ////////////////////////////////
  {
    start = now_micros();
    pool p;
    fast_pooled_ring cb(length);
    for (int i = 0 ; i < num_objects ; ++i) {
      Candle* c = p.construct();
      c->set_timestamp(i);
      cb.push_back(*c);
      p.free(c);
    }

    elapsed = now_micros() - start;
    LOG(INFO) << "pool + fast_pooled_ring: " << num_objects << " objects in "
              << elapsed << " usecs";
  }

  ////////////////////////////////
  {
    start = now_micros();
    pool p;
    unpooled_ring cb(length);
    for (int i = 0 ; i < num_objects ; ++i) {
      Candle* c = p.construct();
      c->set_timestamp(i);
      cb.push_back(*c);
      p.free(c);
    }

    elapsed = now_micros() - start;
    LOG(INFO) << "   pool + unpooled_ring: " << num_objects << " objects in "
              << elapsed << " usecs";
  }
}


void print_array(const int array[], int len)
{
  for (size_t i = 0; i < len; ++i) {
    LOG(INFO) << "array[" << i << "] = " << array[i];
  }
}

void print_array(const boost::circular_buffer<int>& array, int len)
{
  for (size_t i = 0; i < len; ++i) {
    LOG(INFO) << "cb[" << i << "] = " << array[i];
  }
}

TEST(MiscPrototype, CircularBufferAsArray)
{
  size_t len = 5;
  int array[len];
  for (int i = 0; i < len; ++i) {
    array[i] = i * 10;
  }
  print_array(&array[0], len);

  boost::circular_buffer<int> cb(len);
  for (int i = 0; i < len; ++i) {
    cb.push_back(i* 10);
  }
  LOG(INFO) << "Circular Buffer";
  print_array(&cb[0], len);

  LOG(INFO) << "Push back";
  cb.push_back(len * 10);
  print_array(&cb[0], len);
  print_array(cb, len);

  LOG(INFO) << "Conclusion: don't pass circular buffer to an array ";
  LOG(INFO) << "function parameter.  This is a loophole in c++ and ";
  LOG(INFO) << "cannot be used with cicular buffers.";

}

/// It appears that there are some initialization cost associated with
/// the TA lib.  So a trivial case to get the code initialized.
TEST(MiscPrototype, TALibWarmUp)
{
  typedef boost::uint64_t ts;

  size_t len = 1;
  double close[len];
  double sma[len];
  int start = 0;
  int count = 0;

  // initialize
  for (int i = 0; i < len; ++i) {
    close[i] = i * 10.;
    sma[i] = 0.;
  }

  ts s = now_micros();
  int ret = TA_MA(0, len-1, &close[0], 5, TA_MAType_SMA,
                  &start, &count, &sma[0]);
  ts el = now_micros() - s;

  LOG(INFO) << "return = " << ret << ", start=" << start << ", count=" << count
            << " in " << el << " usec.";
  for (int i = 0; i < len-1; ++i) {
    LOG(INFO) << "close[" << i << "] = " << close[i]
              << ", sma[" << i << "] = " << sma[i];
  }
}


TEST(MiscPrototype, TALibCircularBuffer1)
{

  typedef boost::uint64_t ts;

  size_t len = 10;

  boost::circular_buffer<double> close(len);
  double sma[len];
  int start = 0;
  int count = 0;

  // comparison
  double _close[len];
  double _sma[len];
  int _start = 0;
  int _count = 0;

  // initialize
  for (int i = 0; i < len; ++i) {
    close.push_back(i * 10.);
    sma[i] = 0.;
    _close[i] = i* 10.;
    _sma[i] = 0.;
  }

  ts s = now_micros();
  int ret = TA_MA(0, len-1, &close[0], 5, TA_MAType_SMA,
                  &start, &count, &sma[0]);
  ts el = now_micros() - s;
  LOG(INFO) << "return = " << ret << ", start=" << start << ", count=" << count
            << " in " << el << " usec.";

  ts _s = now_micros();
  int _ret = TA_MA(0, len-1, &_close[0], 5, TA_MAType_SMA,
                  &_start, &_count, &_sma[0]);
  ts _el = now_micros() - _s;
  LOG(INFO) << "return = " << _ret
            << ", start=" << _start << ", count=" << _count
            << " in " << _el << " usec.";

  for (int i = 0; i < len; ++i) {
    LOG(INFO) << "close[" << i << "] = " << close[i]
              << ", _close[" << i << "] = " << _close[i]
              << ", sma[" << i << "] = " << sma[i]
              << ", _sma[" << i << "] = " << _sma[i];
  }
  for (int i = 0; i < len; ++i) {
    EXPECT_EQ(_sma[i], sma[i]);
  }

  int k = 5;
  int ta_start = len - k;
  int ta_stop = ta_start;
  // now push one data point
  close.push_back(len * 10.);
  s = now_micros();
  ret = TA_MA(ta_start, ta_stop, &close[0], k, TA_MAType_SMA,
              &start, &count, &sma[0]);
  el = now_micros() - s;
  LOG(INFO) << "ta_start = " << ta_start << ", ta_stop = " << ta_stop
            << ", return = " << ret
            << ", start=" << start << ", count=" << count
            << " in " << el << " usec.";


  for (int i = 0; i < len; ++i) {
    _close[i] += 10; // shift 10 in this special case
  }
  _s = now_micros();
  _ret = TA_MA(ta_start, ta_stop, &_close[0], k, TA_MAType_SMA,
              &_start, &_count, &_sma[0]);
  el = now_micros() - s;
  LOG(INFO) << "ta_start = " << ta_start << ", ta_stop = " << ta_stop
            << ", return = " << _ret
            << ", start=" << _start << ", count=" << _count
            << " in " << _el << " usec.";

  for (int i = 0; i < len; ++i) {
    LOG(INFO) << "close[" << i << "] = " << close[i]
              << ", _close[" << i << "] = " << _close[i]
              << ", sma[" << i << "] = " << sma[i]
              << ", _sma[" << i << "] = " << _sma[i];
  }

  LOG(INFO) << "Note: can't pass a circular buffer into TA-lib functions!";
  LOG(INFO) << "Looks like we'd have to pay the cost of copying.";

}

#include <vector>

#include <boost/assign/std/vector.hpp>
#include <boost/assign.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>

using namespace boost::assign;


struct time_window_policy {
typedef boost::uint64_t microsecond_t;

struct align_at_zero
{

  align_at_zero(const microsecond_t& window_size)
      : window_size_(window_size), mod_ts_(0)
  {
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
    LOG(INFO) << "last_ts = " << last_ts
              << ", timestamp = " << timestamp
              << ", mod_ts = " << mod_ts_
              << ", r=" << r << ", result = " << result;
    mod_ts_ = r;
    return result;
  }

  microsecond_t window_size_;
  microsecond_t mod_ts_;
};

struct by_elapsed_time
{
  by_elapsed_time(const microsecond_t& window_size)
      : window_size_(window_size)
  {
  }

  int count_windows(const microsecond_t& last_ts,
                    const microsecond_t& timestamp)
  {
    return (timestamp - last_ts) / window_size_;
  }

  bool is_new_window(const microsecond_t& last_ts,
                     const microsecond_t& timestamp)
  {
    return (timestamp - last_ts) >= window_size_;
  }

  microsecond_t window_size_;
};
};


TEST(MiscPrototype, TimeWindowPolicyTest)
{
  time_window_policy::align_at_zero p1(100);

  EXPECT_TRUE(p1.is_new_window(0, 1000));
  EXPECT_FALSE(p1.is_new_window(1000, 1000)); // should not advance forward
  EXPECT_FALSE(p1.is_new_window(1000, 1001));
  EXPECT_TRUE(p1.is_new_window(1002, 1101));
  EXPECT_FALSE(p1.is_new_window(1101, 1103));
  EXPECT_FALSE(p1.is_new_window(1100, 1199));
  EXPECT_TRUE(p1.is_new_window(1199, 1200));

  time_window_policy::by_elapsed_time p2(100);
  EXPECT_FALSE(p2.is_new_window(1001, 1001)); // should not advance forward
  EXPECT_FALSE(p2.is_new_window(1001, 1100));
  EXPECT_TRUE(p2.is_new_window(1001, 1101));
  EXPECT_TRUE(p2.is_new_window(1100, 1200));


  EXPECT_EQ(10, p1.count_windows(0, 1000));
  EXPECT_EQ(0, p1.count_windows(1000, 1000)); // should not advance forward
  EXPECT_EQ(0, p1.count_windows(1000, 1001));
  EXPECT_EQ(0, p1.count_windows(1002, 1101));
  EXPECT_EQ(0, p1.count_windows(1101, 1103));
  EXPECT_EQ(0, p1.count_windows(1100, 1199));
  EXPECT_EQ(0, p1.count_windows(1199, 1200));
  EXPECT_EQ(2, p1.count_windows(1200, 1400));

  EXPECT_EQ(0, p2.count_windows(1001, 1001)); // should not advance forward
  EXPECT_EQ(0,p2.count_windows(1001, 1100));
  EXPECT_EQ(1, p2.count_windows(1001, 1101));
  EXPECT_EQ(1, p2.count_windows(1100, 1200));
  EXPECT_EQ(3, p2.count_windows(1100, 1401));

}

template <
  typename element_t,
  typename sampler_t = boost::function<
    element_t(const element_t& last, const element_t& current,
              bool new_sample_period) >,
  typename Alloc = boost::pool_allocator<element_t>,
  typename time_window_policy = time_window_policy::align_at_zero >
class time_series
{
 public:
  typedef boost::uint64_t microsecond_t;
  typedef boost::posix_time::time_duration window_size_t;
  typedef boost::circular_buffer<element_t, Alloc> history_t;
  typedef typename
  boost::circular_buffer<element_t, Alloc>::const_reverse_iterator reverse_itr;


  time_series(boost::posix_time::time_duration h, window_size_t i,
              element_t init) :
      samples_(h.total_microseconds() / i.total_microseconds()),
      interval_(i),
      buffer_(samples_ - 1),
      current_value_(init),
      current_ts_(0),
      time_window_policy_(i.total_microseconds())
  {
    // fill the buffer with the default valule.
    for (size_t i = 0; i < buffer_.capacity(); ++i) {
      buffer_.push_back(init);
    }
  }

  const element_t& operator[](size_t index) const
  {
    return buffer_[index];
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

  template <typename buffer_t>
  const size_t copy_last(buffer_t array[], const size_t length) const
  {
    if (length > buffer_.capacity() + 1) {
      return 0; // No copy is done.
    }
    array[length - 1] = current_value_;

    size_t to_copy = length - 1;
    size_t copied = 1;
    reverse_itr r = buffer_.rbegin();

    // fill the array in reverse order
    for (; r != buffer_.rend() && to_copy > 0; --to_copy, ++copied, ++r) {
      array[length - 1 - copied] = *r;
    }
    return copied;
  }

  /// for testing only
  element_t sample(const element_t& last, const element_t& current)
  {
    return sampler_(last, current, false);
  }

  /// for testing only
  void visit(boost::function< void(const size_t& index,
                                   const element_t& el)> visitor)
  {
    for (size_t i = 0; i < buffer_.capacity(); ++i) {
      visitor(i, buffer_[i]);
    }
    visitor(buffer_.capacity(), current_value_);
  }

  void on(microsecond_t timestamp, element_t value)
  {
    // check to see if the current timestamp falls within the current
    // interval.  If outside, push the current value onto the history buffer
    bool new_sample_period = time_window_policy_.is_new_window(
        current_ts_, timestamp);

    element_t sampled = sampler_(current_value_, value, new_sample_period);
    if (new_sample_period) {

      if (current_ts_ > 0) {
        // Don't fill in missing values if starting up.
        int more = std::max(0, time_window_policy_.count_windows(
            current_ts_, timestamp) - 1);
        LOG(INFO) << "missing = " << more << " windows";
        // TODO- allow filling in missing values like count
        element_t missing = current_value_;
        for (int i = 0; i < more; ++i) {
          buffer_.push_back(missing);
        }
      }

      buffer_.push_back(current_value_);
    }
    current_value_ = sampled;
    current_ts_ = timestamp;
  }

 private:

  size_t samples_;
  window_size_t interval_;
  history_t buffer_;

  element_t current_value_;
  microsecond_t current_ts_;

  sampler_t sampler_;
  time_window_policy time_window_policy_;
};

template <typename element_t>
class count_t
{
 public:
  count_t() : count_(0) {}

  element_t operator()(element_t last, element_t now, bool new_period)
  {
    if (new_period) { count_ = 1; }
    else ++count_;
    return count_;
  }

 private:
  int count_;
};

template <typename element_t>
class current_t
{
 public:

  element_t operator()(element_t last, element_t now, bool new_period)
  {
    return now;
  }
};

template <typename element_t>
class open_t
{
 public:

  open_t() : init_(false) {}

  element_t operator()(element_t last, element_t now, bool new_period)
  {
    if (new_period) { open_ = now; init_ = true; }
    if (!init_) { open_ = now; init_ = true; }
    return open_;
  }

 private:
  element_t open_;
  bool init_;
};

template <typename element_t>
class close_t
{
 public:

  element_t operator()(element_t last, element_t now, bool new_period)
  {
    return last;
  }
};

template <typename element_t>
class max_t
{
 public:
  element_t operator()(element_t last, element_t now, bool new_period)
  {
    if (new_period) return now;
    else return std::max(last, now);
  }
};

template <typename element_t>
class min_t
{
 public:
  element_t operator()(element_t last, element_t now, bool new_period)
  {
    if (new_period) return now;
    else return std::min(last, now);
  }
};



TEST(MiscPrototype, TimeSeries1)
{
  using boost::posix_time::time_duration;
  using boost::posix_time::minutes;
  using boost::posix_time::seconds;
  using boost::fast_pool_allocator;

  using proto::test::Candle;

  time_duration one_min = minutes(1);
  time_duration one_sec = seconds(1);

  time_series< double, max_t<double> > h1(minutes(10), minutes(1), 0.);
  time_series< int, min_t<int> > h2(seconds(10), seconds(1), 0);

  boost::uint64_t now = now_micros();
  Candle init;
  time_series< Candle,
               fast_pool_allocator<Candle> > h3(minutes(10), seconds(2), init);

  boost::uint64_t el = now_micros() - now;
  LOG(INFO) << "elapsed " << el << ", h3 = " << h3.capacity();

  EXPECT_EQ(one_min.total_microseconds(), h1.sample_microseconds());
  EXPECT_EQ(one_sec.total_microseconds(), h2.sample_microseconds());
  EXPECT_EQ(one_sec.total_microseconds() * 2, h3.sample_microseconds());

  EXPECT_EQ(10, h1.capacity());
  EXPECT_EQ(10, h2.capacity());
  EXPECT_EQ(60 * 10 / 2, h3.capacity());

  LOG(INFO) << "here";
  h2.on(now_micros(), 1);

  EXPECT_EQ(100., h1.sample(100., 1.));
  EXPECT_EQ(1, h2.sample(100, 1));

}

template <typename T>
struct Verifier
{
  Verifier(const std::vector<T>& expectations) : expectations(expectations)
  {
  }

  void operator()(const size_t& index, const T& value) {
    LOG(INFO) << "index = " << index << ", value = " << value;
    EXPECT_EQ(expectations[index], value);
  }

  const std::vector<T>& expectations;
};

TEST(MiscPrototype, TimeSeries2)
{
  using boost::posix_time::time_duration;
  using boost::posix_time::microseconds;
  using namespace boost::assign;  // brings in the += operator for vector

  time_series< int, current_t<int> > h(microseconds(100), microseconds(10), 0);
  time_series< int, count_t<int> > h2(microseconds(100), microseconds(10), 0);
  time_series< int, open_t<int> > h3(microseconds(100), microseconds(10), 0);

  LOG(INFO) << "Checking h";
  h.on(999, 10);
  h.on(1000, 1);
  h.on(1001, 2);
  h.on(1005, 3);
  h.on(1009, 4);
  h.on(1010, 5);
  h.on(1011, 6);
  h.on(1012, 7);
  h.on(1052, 100);

  std::vector<int> expect;
  expect += 0, 0, 0, 10, 4, 7, 7, 7, 7, 100;
  assert(expect.size() == 10);
  Verifier<int> f(expect);
  h.visit(f);

  LOG(INFO) << "Checking h2";
  h2.on(998, 10);
  h2.on(999, 10);
  h2.on(1000, 1);
  h2.on(1001, 2);
  h2.on(1005, 3);
  h2.on(1009, 4);
  h2.on(1010, 5);
  h2.on(1011, 6);
  h2.on(1012, 7);

  std::vector<int> expect2;
  expect2 += 0, 0, 0, 0, 0, 0, 0, 2, 4, 3;
  Verifier<int> f2(expect2);
  h2.visit(f2);

  LOG(INFO) << "Checking h3";
  h3.on(999, 10);
  h3.on(1000, 1);
  h3.on(1001, 2);
  h3.on(1005, 3);
  h3.on(1009, 4);
  h3.on(1010, 5);
  h3.on(1011, 6);
  h3.on(1012, 7);
  h3.on(1042, 10); // after multiple time windows.

  std::vector<int> expect3;
  expect3 += 0, 0, 0, 0, 10, 1, 5, 5, 5, 10;
  assert(expect3.size() == 10); // 10 samples in history!
  Verifier<int> f3(expect3);
  h3.visit(f3);

  //expect += 0, 0, 0, 10, 4, 7, 7, 7, 7, 100;

  {
    LOG(INFO) << "buffer test";
    int buff[10];
    size_t copied = h.copy_last(buff, 10);
    EXPECT_EQ(10, copied);

    for (int i = 0; i < 10; ++i) {
      EXPECT_EQ(expect[i], buff[i]);
    }
  }

  {
    LOG(INFO) << "buffer test";
    int buff[3];
    size_t copied = h.copy_last(buff, 3);
    EXPECT_EQ(3, copied);

    for (int i = 0; i < 3; ++i) {
      EXPECT_EQ(expect[7 + i], buff[i]);
    }
  }

  {
    LOG(INFO) << "buffer test";
    float buff[6];
    size_t copied = h.copy_last(buff, 6);
    EXPECT_EQ(6, copied);

    for (int i = 0; i < 6; ++i) {
      EXPECT_EQ(expect[4 + i], buff[i]);
    }
  }

}


TEST(MiscPrototype, TimeSeries3)
{
  time_series< double, current_t<double> > last_trade(
      microseconds(1000), microseconds(10), 0.);

  last_trade.on(10001, 10.); // 10.
  last_trade.on(10011, 11.);
  last_trade.on(10012, 12.); // 12.
  last_trade.on(10022, 13.);
  last_trade.on(10029, 15.);
  last_trade.on(10029, 17.); // 17.
  last_trade.on(10030, 20.);
  last_trade.on(10031, 21.); // 21., 21., 21., 21.,
  last_trade.on(10072, 22.);
  last_trade.on(10074, 25.); // 25., 25.
  last_trade.on(10095, 20.); // 20.

  std::vector<double> p;
  p += 10., 12., 17., 21., 21., 21., 21., 25., 25., 20.;

  double buff[p.size()];
  int copied = last_trade.copy_last(buff, p.size());

  EXPECT_EQ(p.size(), copied);

  for (int i = 0; i < p.size(); ++i) {
    EXPECT_EQ(p[i], buff[i]);
  }
}
