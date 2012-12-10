
#include <string>
#include <iostream>

#include <curl/curl.h>


#include <gflags/gflags.h>

#include "log_levels.h"
#include "common/time_utils.hpp"


#include "couchdb/couchdb.hpp"

using std::string;
namespace josn = json_spirit;


namespace atp {
namespace couchdb {

class couchdb::implementation
{
 public:
  explicit implementation(const string& host,
                          unsigned int port,
                          const string& db) :
      host_(host), port_(port), db_(db)
  {
    std::ostringstream oss;
    oss << "http://" << host_ << ':' << port_ << '/' << db;
    url_prefix_ = oss.str();
  }

  const string& url_prefix() const
  {
    return url_prefix_;
  }

 private:
  const string host_;
  const unsigned int port_;
  const string db_;
  string url_prefix_;
};



//////////
couchdb::couchdb(const string& host, unsigned int port, const string& db) :
    impl_(new implementation(host, port, db))
{
}

couchdb::~couchdb()
{
}

const string& couchdb::url_prefix() const
{
  return impl_->url_prefix();
}

} // couchdb
} // atp
