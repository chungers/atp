

#include <string>

#include <boost/function.hpp>
#include <boost/unordered_map.hpp>

#include <boost/circular_buffer.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/pool/pool_alloc.hpp>
#include <boost/pool/singleton_pool.hpp>
#include <gtest/gtest.h>
#include <glog/logging.h>

#include "utils.hpp"

#include "test.pb.h"

#include "common/Executor.hpp"
#include "common/factory.hpp"
#include "zmq/ZmqUtils.hpp"

using std::string;
using ::zmq::context_t;
using ::zmq::error_t;
using ::zmq::socket_t;
using atp::common::factory;

template <typename Context>
struct Handler
{
  typedef unsigned int status_t;

  static const status_t CANNOT_PARSE    = 100;
  static const status_t NOT_INITIALIZED = 101;

  virtual status_t Execute(Context& context, const string& message) = 0;
};

template <typename ProtoBufferMessage,
          typename Context,
          class Processor =
          boost::function< unsigned int(const ProtoBufferMessage& message,
                                        Context& context) > >
class MessageHandler : Handler<Context>
{
 public:

  typedef unsigned int status_t;

  MessageHandler(Processor processor) : processor_(processor) {}
  virtual ~MessageHandler() {}

  virtual status_t Execute(Context& context,
                           const string& message)
  {
    message_.Clear();

    if (message_.ParseFromString(message)) {
      if (message_.IsInitialized()) {
        return processor_(message_, context);
      } else {
        return Handler<Context>::NOT_INITIALIZED;
      }
    } else {
      return Handler<Context>::CANNOT_PARSE;
    }
  }

 private:
  ProtoBufferMessage message_;
  Processor processor_;
};

/// Basic message listener that handles the message based on
/// a topic followed by a protobuffer string
template <typename Context>
class MessageListener
{
 public:

  MessageListener(const string& endpoint, context_t* context,
                  factory<Handler<Context> >& factory) :
      endpoint_(endpoint),
      context_(context),
      factory_(factory)
  {
  }

 private:

  string endpoint_;
  context_t* context_;
  factory< Handler<Context> >& factory_;
};

class Agent
{

};




TEST(AgentPrototype, CircularBuffer1)
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


TEST(AgentPrototype, CircularBuffer2)
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

TEST(AgentPrototype, CircularBufferTiming)
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
TEST(AgentPrototype, ObjectPool1)
{
  using proto::test::Candle;
  typedef boost::singleton_pool<CandleTag, sizeof(Candle)> singleton_pool;

  typedef boost::object_pool<Candle> pool;
  typedef boost::uint64_t ts;

  size_t num_objects = 10000000;
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

TEST(AgentPrototype, CircularBufferWithPoolAllocator)
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
  size_t num_objects = 10000000;
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
