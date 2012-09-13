

#include <string>

#include <boost/circular_buffer.hpp>
#include <boost/pool/object_pool.hpp>
#include <gtest/gtest.h>
#include <glog/logging.h>

#include "utils.hpp"

#include "test.pb.h"



TEST(IndicatorPrototype, CircularBuffer1)
{
  boost::circular_buffer<int> cb(3);
  cb.push_back(1);
  cb.push_back(2);
  cb.push_back(3);

  EXPECT_EQ(1, cb[0]);
  EXPECT_EQ(2, cb[1]);
  EXPECT_EQ(3, cb[2]);

  cb.push_back(4);
  EXPECT_EQ(2, cb[0]);
  EXPECT_EQ(3, cb[1]);
  EXPECT_EQ(4, cb[2]);

  cb.push_back(5);
  cb.push_back(6);

  EXPECT_EQ(4, cb[0]);
  EXPECT_EQ(5, cb[1]);
  EXPECT_EQ(6, cb[2]);
}


TEST(IndicatorPrototype, CircularBuffer2)
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

TEST(IndicatorPrototype, CircularBufferTiming)
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
