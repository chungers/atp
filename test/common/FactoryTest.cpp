
#include <string>

#include <gtest/gtest.h>
#include <glog/logging.h>

#include "utils.hpp"
#include "ib/ZmqMessage.hpp"
#include "ib/api966/ApiMessages.hpp"

#include "common/factory.hpp"

using std::string;
using atp::common::factory;
using ib::internal::ZmqMessage;
using IBAPI::V966::CancelMarketData;
using IBAPI::V966::RequestMarketData;
using IBAPI::V966::CancelMarketDepth;
using IBAPI::V966::RequestMarketDepth;

namespace p = proto::ib;


// Function template for parsing string data and creating
// zmq message objects.
template <class M>
M* create(const string& key, const string& msg)
{
  M* ptr = new M();
  string protoKey = ptr->key();

  LOG(INFO) << "proto key = " << protoKey << ", actual = " << key;
  if (key == protoKey && ptr->ParseFromString(msg)) {
    return ptr;
  }
  return NULL;
}


p::RequestMarketData REQUEST_MARKET_DATA;
p::CancelMarketData CANCEL_MARKET_DATA;
p::RequestMarketDepth REQUEST_MARKET_DEPTH;
p::CancelMarketData CANCEL_MARKET_DEPTH;


factory< ZmqMessage > *InitFactory()
{
  factory< ZmqMessage > *f = new factory< ZmqMessage >();

  f->register_creator(REQUEST_MARKET_DATA.GetTypeName(),
                      &create<RequestMarketData>);

  f->register_creator(CANCEL_MARKET_DATA.GetTypeName(),
                      &create<CancelMarketData>);

  f->register_creator(REQUEST_MARKET_DEPTH.GetTypeName(),
                      &create<RequestMarketDepth>);

  f->register_creator(CANCEL_MARKET_DEPTH.GetTypeName(),
                      &create<CancelMarketDepth>);

  return f;
}


TEST(FactoryTest, FactoryBasicTest)
{
  factory< ZmqMessage > *factory = InitFactory();

  EXPECT_TRUE(factory->is_supported(CANCEL_MARKET_DEPTH.GetTypeName()));
  EXPECT_TRUE(factory->is_supported(REQUEST_MARKET_DEPTH.GetTypeName()));
  EXPECT_TRUE(factory->is_supported(CANCEL_MARKET_DATA.GetTypeName()));
  EXPECT_TRUE(factory->is_supported(REQUEST_MARKET_DATA.GetTypeName()));
  EXPECT_FALSE(factory->is_supported("unknown"));

  delete factory;
}


TEST(FactoryTest, FactoryCreateObjectTest)
{
  factory< ZmqMessage > *factory = InitFactory();

  ZmqMessage *req;

  req = factory->create_object(REQUEST_MARKET_DATA.GetTypeName(),
                              "bad data");
  EXPECT_TRUE(req == NULL);

  p::RequestMarketData r;
  boost::uint64_t t = now_micros();
  r.set_timestamp(t);
  r.set_message_id(t);
  r.mutable_contract()->set_id(1000);
  r.mutable_contract()->set_symbol("AAPL");


  LOG(INFO) << "RequestMarketData: " << r.GetTypeName()
            << ", size = " << r.ByteSize();


  EXPECT_TRUE(r.IsInitialized());

  std::string message;
  r.SerializeToString(&message);

  EXPECT_TRUE(r.ParseFromString(message));

  req = factory->create_object(REQUEST_MARKET_DATA.GetTypeName(),
                               message);
  EXPECT_TRUE(req != NULL);  //  should create ok
  EXPECT_EQ(r.GetTypeName(), req->key());

  delete factory;
}

