
#include <string>

#include <gtest/gtest.h>
#include <glog/logging.h>

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

template <class M>
M* create(const string& msg)
{
  M* ptr = new M();
  if (ptr->ParseFromString(msg)) {
    return ptr;
  }
  return NULL;
}


TEST(FactoryTest, FactoryBasicTest)
{

  Factory< ZmqMessage, string > factory;

  p::RequestMarketData REQUEST_MARKET_DATA;
  factory.Register(REQUEST_MARKET_DATA.key(), &create<RequestMarketData>);

  p::CancelMarketData CANCEL_MARKET_DATA;
  factory.Register(CANCEL_MARKET_DATA.key(), &create<CancelMarketData>);

  p::RequestMarketDepth REQUEST_MARKET_DEPTH;
  factory.Register(REQUEST_MARKET_DEPTH.key(), &create<RequestMarketDepth>);

  p::CancelMarketData CANCEL_MARKET_DEPTH;
  factory.Register(CANCEL_MARKET_DEPTH.key(), &create<CancelMarketDepth>);

  EXPECT_TRUE(factory.IsSupported(CANCEL_MARKET_DEPTH.key()));
  EXPECT_TRUE(factory.IsSupported(REQUEST_MARKET_DEPTH.key()));
  EXPECT_TRUE(factory.IsSupported(CANCEL_MARKET_DATA.key()));
  EXPECT_TRUE(factory.IsSupported(REQUEST_MARKET_DATA.key()));
  EXPECT_FALSE(factory.IsSupported("unknown"));

}
