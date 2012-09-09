
#include <string>

#include <gtest/gtest.h>
#include <glog/logging.h>

#include "utils.hpp"
#include "ib/ZmqMessage.hpp"
#include "ib/api966/ApiMessages.hpp"

#include "common/Factory.hpp"

using std::string;
using atp::common::Factory;
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


Factory< ZmqMessage > *InitFactory()
{
  Factory< ZmqMessage > *factory = new Factory< ZmqMessage >();

  factory->Register(REQUEST_MARKET_DATA.GetTypeName(), &create<RequestMarketData>);

  factory->Register(CANCEL_MARKET_DATA.GetTypeName(), &create<CancelMarketData>);

  factory->Register(REQUEST_MARKET_DEPTH.GetTypeName(), &create<RequestMarketDepth>);

  factory->Register(CANCEL_MARKET_DEPTH.GetTypeName(), &create<CancelMarketDepth>);

  return factory;
}


TEST(FactoryTest, FactoryBasicTest)
{
  Factory< ZmqMessage > *factory = InitFactory();

  EXPECT_TRUE(factory->IsSupported(CANCEL_MARKET_DEPTH.GetTypeName()));
  EXPECT_TRUE(factory->IsSupported(REQUEST_MARKET_DEPTH.GetTypeName()));
  EXPECT_TRUE(factory->IsSupported(CANCEL_MARKET_DATA.GetTypeName()));
  EXPECT_TRUE(factory->IsSupported(REQUEST_MARKET_DATA.GetTypeName()));
  EXPECT_FALSE(factory->IsSupported("unknown"));

  delete factory;
}


TEST(FactoryTest, FactoryCreateObjectTest)
{
  Factory< ZmqMessage > *factory = InitFactory();

  ZmqMessage *req;

  req = factory->CreateObject(REQUEST_MARKET_DATA.GetTypeName(),
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

  req = factory->CreateObject(REQUEST_MARKET_DATA.GetTypeName(),
                              message);
  EXPECT_TRUE(req != NULL);  //  should create ok
  EXPECT_EQ(r.GetTypeName(), req->key());

  delete factory;
}

