
#include <string>

#include <gtest/gtest.h>
#include <glog/logging.h>

#include "common/time_utils.hpp"
#include "couchdb/couchdb.hpp"

namespace couch = atp::couchdb;

TEST(CouchDbTest, Usage1)
{
  couch::couchdb db("127.0.0.1", 5984, "dev");

  EXPECT_EQ("http://127.0.0.1:5984/dev", db.url_prefix());
}
