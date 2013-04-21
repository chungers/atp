
#include <iostream>
#include <string>

#include <gtest/gtest.h>
#include <glog/logging.h>

#include "common/time_utils.hpp"
#include "couchdb/couchdb.hpp"

namespace couch = atp::couchdb;

using couch::couchdb;

TEST(CouchDbTest, Usage1)
{
  couch::couchdb db("127.0.0.1", 5984, "dev");

  EXPECT_EQ("http://127.0.0.1:5984/dev", db.url_prefix());

  couchdb::json_string json("{\"name\":\"test\"}");

  std::ostringstream key;
  key << "CouchDbTest_Usage1_" << now_micros();

  couchdb::raw_response resp = db.put(key.str(), json);

  EXPECT_EQ(couchdb::NEW, resp.first);
  LOG(INFO) << "Response = " << resp.first << ", " << resp.second;

  // try to read it
  couchdb::raw_response get_resp = db.get(key.str());
  EXPECT_EQ(couchdb::GET_OK, get_resp.first);
  LOG(INFO) << "GET response = " << get_resp.first << ", " << get_resp.second;

}
