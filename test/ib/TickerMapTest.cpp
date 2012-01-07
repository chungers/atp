
#include <map>
#include <iostream>
#include <sys/time.h>
#include <set>
#include <vector>

#include <boost/algorithm/string.hpp>

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <gtest/gtest.h>

#include "ib/TickerMap.hpp"

using namespace std;
using namespace boost;
using namespace ib::internal;


namespace {

TEST(TickerMapTest, ConvertToContractTest)
{
  using namespace std;

  // Create a map that has the attributes
  map<string, string> nv;
  nv["conId"] = "12345";
  nv["symbol"] = "AAPL";
  nv["secType"] = "STK";
  nv["localSymbol"] = "";
  nv["currency"] = "USD";

  Contract c;
  bool converted = convert_to_contract(nv, &c);

  EXPECT_TRUE(converted);
  EXPECT_EQ(12345, c.conId);
  EXPECT_EQ("AAPL", c.symbol);
  EXPECT_EQ("", c.localSymbol);
  EXPECT_EQ("STK", c.secType);
  EXPECT_EQ("USD", c.currency);
}

} // Namespace
