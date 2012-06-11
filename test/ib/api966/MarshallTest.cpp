
#include <glog/logging.h>
#include <gtest/gtest.h>

#include "ib/api966/marshall.hpp"
#include "ib/api966/ostream.hpp"

namespace IBAPI {
namespace V966 {


TEST(V966_MarshallTest, STKProtoToContractConversion)
{
  proto::ib::Contract p;
  Contract c;

  p.set_id(100);
  p.set_symbol("AAPL");

  EXPECT_TRUE(p.IsInitialized());
  EXPECT_TRUE(p >> c);
  EXPECT_EQ("AAPL", c.symbol);
  EXPECT_EQ(100, c.conId);
  EXPECT_EQ("SMART", c.exchange); // default
  EXPECT_EQ("STK", c.secType); // default
}

TEST(V966_MarshallTest, OptionProtoToContractConversion)
{
  proto::ib::Contract p;
  Contract c;

  p.set_id(100);
  p.set_symbol("AAPL");
  p.set_type(proto::ib::Contract::OPTION);
  p.set_right(proto::ib::Contract::CALL);
  p.mutable_strike()->set_amount(500.0);
  p.mutable_expiry()->set_year(2012);
  p.mutable_expiry()->set_month(5);
  p.mutable_expiry()->set_day(5);

  EXPECT_TRUE(p.IsInitialized());
  EXPECT_TRUE(p >> c);
  EXPECT_EQ("AAPL", c.symbol);
  EXPECT_EQ(100, c.conId);
  EXPECT_EQ("SMART", c.exchange); // default
  EXPECT_EQ("OPT", c.secType);
  EXPECT_EQ("C", c.right);
  EXPECT_EQ(500.0, c.strike);
  EXPECT_EQ("20120505", c.expiry);
}

TEST(V966_MarshallTest, STKContractToProtoConversion)
{
  proto::ib::Contract p;
  Contract c;

  c.conId = 100;
  c.symbol = "AAPL";

  EXPECT_TRUE(c >> p);
  EXPECT_TRUE(p.IsInitialized());
  EXPECT_EQ("AAPL", p.symbol());
  EXPECT_EQ(100, p.id());
  EXPECT_EQ("SMART", p.exchange()); // default
  EXPECT_EQ(proto::ib::Contract::STOCK, p.type()); // default

  // Do the inverse
  Contract c2;
  p >> c2;
  EXPECT_EQ(c.conId, c2.conId);
  EXPECT_EQ(c.symbol, c2.symbol);
  EXPECT_EQ(c.strike, c2.strike);
  EXPECT_EQ(c.expiry, c2.expiry);
  EXPECT_EQ(c.right, c2.right);

  // Default set by the protobuff definitions even if they are not set in the
  // ib Contract struct.
  EXPECT_EQ("SMART", c2.exchange);
  EXPECT_EQ("STK", c2.secType);
  EXPECT_EQ("USD", c2.currency);

  LOG(INFO) << "c1 = " << c;
  LOG(INFO) << "c2 = " << c2;
}

TEST(V966_MarshallTest, OptionContractToProtoConversion)
{
  proto::ib::Contract p;
  Contract c;

  c.conId = 100;
  c.symbol = "AAPL";
  c.strike = 500;
  c.right = "P";
  c.expiry = "20120505";
  c.secType = "OPT";

  EXPECT_TRUE(c >> p);
  EXPECT_TRUE(p.IsInitialized());
  EXPECT_EQ("AAPL", p.symbol());
  EXPECT_EQ(100, p.id());
  EXPECT_EQ("SMART", p.exchange()); // default
  EXPECT_EQ(proto::ib::Contract::OPTION, p.type());
  EXPECT_EQ(proto::ib::Contract::PUT, p.right());
  EXPECT_EQ(500, p.strike().amount());
  EXPECT_EQ(proto::common::Money::USD, p.strike().currency());
  EXPECT_EQ(2012, p.expiry().year());
  EXPECT_EQ(5, p.expiry().month());
  EXPECT_EQ(5, p.expiry().day());

  // Do the inverse
  Contract c2;
  p >> c2;
  EXPECT_EQ(c.conId, c2.conId);
  EXPECT_EQ(c.symbol, c2.symbol);
  EXPECT_EQ(c.strike, c2.strike);
  EXPECT_EQ(c.expiry, c2.expiry);
  EXPECT_EQ(c.right, c2.right);
  EXPECT_EQ(c.secType, c2.secType);

  EXPECT_EQ("SMART", c2.exchange);
  EXPECT_EQ("USD", c2.currency);

  LOG(INFO) << "c1 = " << c;
  LOG(INFO) << "c2 = " << c2;
}

} // V966
} // IBAPI

