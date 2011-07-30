
#include <map>
#include <iostream>
#include <sys/time.h>
#include <set>
#include <vector>

#include <boost/algorithm/string.hpp>

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <gtest/gtest.h>

#include "utils.hpp"
#include "ib/ticker_id.hpp"

using namespace std;
using namespace boost;
using namespace ib::internal;


DEFINE_int32(iter, 1000000, "Iterations");

namespace {


TEST(UtilsTest, Test1)
{
  int iterations = FLAGS_iter;
  uint64_t now = now_micros();

  for (int i = 0; i < iterations; ++i) {
    string n1;
    now_micros(&n1);
  }

  uint64_t elapsed = now_micros() - now;
  LOG(INFO) << "CREATE AS STRING:"
            << " iterations=" << iterations
            << " dt=" << elapsed
            << " qps=" << (iterations / elapsed * 1000000ULL)
            << endl;

  ////////////////////////////////////////
  now = now_micros();
  for (int i = 0; i < iterations; ++i) {
    now_micros();
  }

  elapsed = now_micros() - now;
  LOG(INFO) << "CREATE AS UINT64:"
            << " iterations=" << iterations
            << " dt=" << elapsed
            << " qps=" << (iterations / elapsed * 1000000ULL)
            << endl;
}


TEST(UtilsTest, TestTsParsing)
{
  int iterations = FLAGS_iter;

  ////////////////////////////////////////
  // First compute cost of creating timestamps.
  uint64_t start = now_micros();
  for (int i = 0; i < iterations; ++i) {
    const char *timestamp;
    {
      uint64_t ts = now_micros();
      stringstream ss;
      ss << ts;
      const string ts_copy(ss.str()); // make a copy of the temp string from ss
      timestamp = ts_copy.c_str();
    }
  }
  uint64_t cost = now_micros() - start;
  LOG(INFO) << "Cost of creating timestamps:"
            << " cost=" << cost
            << endl;

  // Now generate timestamps and perform conversion:
  start = now_micros();
  for (int i = 0; i < iterations; ++i) {
    const char *timestamp;
    uint64_t ts = 0;
    {
      ts = now_micros();
      stringstream ss;
      ss << ts;
      const string ts_copy(ss.str()); // make a copy of the temp string from ss
      timestamp = ts_copy.c_str();
    }

    char* end_ptr;
    uint64_t ts2 = static_cast<uint64_t>(strtoll(timestamp, &end_ptr, 10));
    EXPECT_EQ(ts, ts2);
  }

  uint64_t dt = now_micros() - start;
  uint64_t actual = dt - cost;
  LOG(INFO) << "Conversion:"
            << " iterations=" << iterations
            << " cost=" << cost
            << " dt=" << dt
            << " actual=" << actual
            << " qps=" << (iterations / actual * 1000000)
            << endl;
}

TEST(UtilsTest, TestStringToUpper)
{
  EXPECT_EQ("AAPL", ::to_upper("aapl"));
  EXPECT_EQ("AAPL", ::to_upper("Aapl"));
  EXPECT_EQ("AAPL", ::to_upper("AaPl"));
  EXPECT_EQ("AAPL", ::to_upper("aapL"));
  EXPECT_EQ("AAPL", ::to_upper("AAPL"));
  EXPECT_EQ("XYZ", ::to_upper("XYZ"));
  EXPECT_EQ("XYZ", ::to_upper("xyz"));
}

TEST(UtilsTest, TestStringToLower)
{
  EXPECT_EQ("xyz", ::to_lower("xyz"));
  EXPECT_EQ("xyz", ::to_lower("XYZ"));
}

static string symbols =
    "AAAA,AAAB,ABBB,ZZZZ,ZZZY,\
FAS,FAZ,SRS,GS,AAPL,GOOG,URE,RIMM,TBT,BAC,DXD,\
DXO,FSLR,FXP,LDK,QID,QLD,REW,SDS,SKF,BK,JPM,\
MS,SMN,SSO,TYH,TYP,UYM,XLE,XLV,AYI,AMZN,DDM,\
C,COF,AXP,RTH";

int test_encode(const string& symbol)
{
  int code = SymbolToTickerId(symbol);
  string from_code;
  SymbolFromTickerId(code, &from_code);
  EXPECT_EQ(to_upper(symbol), from_code);
  return code;
}


int test_encode_option(const string& symbol, bool callOption,
                       double strike)
{
  int code = SymbolToTickerId(symbol, callOption, strike);
  string from_code;
  SymbolFromTickerId(code, &from_code);
  EXPECT_EQ(to_upper(symbol), from_code);
  return code;
}

// From ticker_id.cpp:
static const int OFFSET = 11;
static const int MAX_OPTION_PART = 1 << (OFFSET + 1) - 1;
static const int MID = 1 << (OFFSET - 1);

TEST(UtilsTest, PrintConstants)
{
  using namespace std;

  cout << "MID = " << MID << endl;
  cout << "shifted= " << (1 << OFFSET) << endl;
  cout << "maxOpt= " << (MAX_OPTION_PART) << endl;
  cout << "AAAA = " << SymbolToTickerId("AAAA") << endl;
  cout << "ZZZZ = " << SymbolToTickerId("ZZZZ") << endl;
}

TEST(UtilsTest, TestEncodingAndDecodingTickerId)
{
  vector<string> list;
  vector<string>::iterator itr;

  boost::split(list, symbols, boost::is_any_of(","));

  for (itr = list.begin(); itr != list.end(); itr++) {
    string s = *itr;
    EXPECT_EQ(test_encode(s), test_encode(::to_lower(s)));
  }
}

TEST(UtilsTest, TestEncodingOptions)
{
  vector<string> list;
  vector<string>::iterator itr;
  double strike(10.0);

  boost::split(list, symbols, boost::is_any_of(","));
  for (itr = list.begin(); itr != list.end(); itr++) {
    string s = *itr;
    int a = test_encode_option(s, true, strike);
    int b = test_encode_option(::to_lower(s), true, strike);
    EXPECT_EQ(a, b);
  }

  strike = 1023;
  bool call = true;
  int i = 0;
  for (itr = list.begin(); itr != list.end(); itr++, i++) {
    string s = *itr;
    int a = SymbolToTickerId(s, call, strike);
    EncodedOption eo;
    EncodedOptionFromTickerId(a, &eo);
    EXPECT_EQ(s, eo.symbol);
    EXPECT_EQ(call, eo.call_option);
    EXPECT_EQ(strike, eo.strike);
  }

  strike = 1023;
  call = false;
  for (itr = list.begin(); itr != list.end(); itr++, i++) {
    string s = *itr;
    int a = SymbolToTickerId(s, call, strike);
    EncodedOption eo;
    EncodedOptionFromTickerId(a, &eo);
    EXPECT_EQ(s, eo.symbol);
    EXPECT_EQ(call, eo.call_option);
    EXPECT_EQ(strike, eo.strike);
  }
}

TEST(UtilsTest, TestEncodingOptions2)
{
  vector<string> list;
  vector<string>::iterator itr;
  double strike(1.0);
  double increment(1.00);
  double maxStrike(1000.00);

  boost::split(list, symbols, boost::is_any_of(","));

  // Store the generated codes in a set to check for collision.
  set<int> codes;

  int i = 0;
  for (itr = list.begin(); itr != list.end(); itr++, i++) {
    strike = 1.0;

    string s = *itr;

    while (strike <= maxStrike) {
      int a = SymbolToTickerId(s, true, strike);

      EXPECT_TRUE(codes.end() == codes.find(a));
      codes.insert(a);

      //cout << "code = " << a << ", strike = " << strike << endl;

      EncodedOption eo;
      EncodedOptionFromTickerId(a, &eo);
      EXPECT_EQ(s, eo.symbol);
      EXPECT_EQ(true, eo.call_option);
      EXPECT_EQ(strike, eo.strike);

      strike += increment;
    }

    // work backwards now
    while (strike >= 1.0) {

      int a = SymbolToTickerId(s, false, strike);
      //cout << "code = " << a << ", strike = " << strike << endl;

      EXPECT_TRUE(codes.end() == codes.find(a));
      codes.insert(a);

      EncodedOption eo;
      EncodedOptionFromTickerId(a, &eo);
      EXPECT_EQ(s, eo.symbol);
      EXPECT_EQ(false, eo.call_option);
      EXPECT_EQ(strike, eo.strike);

      strike -= increment;
    }
  }
}

} // Namespace
