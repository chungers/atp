
#include <cassert>
#include <stdio.h>
#include <string>
#include <iostream>
#include <vector>

#include <glog/logging.h>
#include <gtest/gtest.h>

#include <json_spirit.h>

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include "varz/varz.hpp"
#include "varz/VarzServer.hpp"

using namespace std;
using namespace atp::varz;


DEFINE_VARZ_int64(start_ts, 0, "Start timestamp");
DEFINE_VARZ_string(name, "", "Name");
DEFINE_VARZ_int32(count, 0, "count");
DEFINE_VARZ_bool(flag, false, "flag");

TEST(VarzTest, VarzServerStartStopTest)
{
  VarzServer vs(8888, 2);
  LOG(INFO) << "Starting server";
  vs.start();
  sleep(1);
  LOG(INFO) << "Stopping server explicitly";
  vs.stop();
}

TEST(VarzTest, VarzServerStartStopTest2)
{
  VarzServer vs(8888, 2);
  LOG(INFO) << "Starting server";
  vs.start();
  sleep(1);
  LOG(INFO) << "Stopping server when server out of scope.";
}

static bool curl(const string& url, string *output)
{
  // Make a curl call
  string command = "curl \"" + url + "\"";
  FILE *f = popen(command.c_str(), "r");
  if (f == NULL) {
    return false;
  }

  ostringstream oss;
  const int BUFSIZE = 1000;
  char buf[BUFSIZE];
  while(fgets(buf, BUFSIZE, f)) {
    oss << buf;
  }
  pclose(f);
  *output = oss.str();
  return true;
}


const json_spirit::mValue& find_value(
    const json_spirit::mObject& obj, const string& name)
{
  try {

    json_spirit::mObject::const_iterator i = obj.find( name );
    assert( i != obj.end() );
    assert( i->first == name );
    LOG(INFO) << "type = " << i->second.type();
    return i->second;

  } catch (std::runtime_error e) {
    LOG(ERROR) << "error: " << e.what();
    throw e;
  }
}

TEST(VarzTest, CurlTest)
{
  VarzServer vs(8888, 2);
  LOG(INFO) << "Starting server";
  vs.start();

  VARZ_name = "test1";

  sleep(1);

  const string url = "http://localhost:8888/varz";
  string result;

  bool success = curl(url, &result);
  EXPECT_TRUE(success);
  LOG(INFO) << "Result = " << result;

  json_spirit::Value value;
  bool read = json_spirit::read(result, value);
  EXPECT_TRUE(read);

  try {
    string name = json_spirit::find_value(value.get_obj(), "name").get_str();
    EXPECT_EQ("test1", name);
  } catch (...) { FAIL(); }
  try {
    int32 count = json_spirit::find_value(value.get_obj(), "count").get_int();
    EXPECT_EQ(0, count);
  } catch (...) { FAIL(); }
  try {
    bool flag = json_spirit::find_value(value.get_obj(), "flag").get_bool();
    EXPECT_EQ(false, flag);
  } catch (...) { FAIL(); }

  VARZ_count++;
  success = curl(url, &result);
  EXPECT_TRUE(success);
  LOG(INFO) << "Result = " << result;

  json_spirit::Value value2;
  bool read2 = json_spirit::read(result, value2);
  EXPECT_TRUE(read2);
  int32 count2 = json_spirit::find_value(value2.get_obj(), "count").get_int();
  EXPECT_EQ(1, count2);

  LOG(INFO) << "Stopping server when server out of scope.";
}

