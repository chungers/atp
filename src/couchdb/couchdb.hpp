#ifndef ATP_COUCHDB_H_
#define ATP_COUCHDB_H_

#include <string>
#include <boost/scoped_ptr.hpp>
#include <json_spirit.h>

#include "common.hpp"


using std::string;
namespace json = json_spirit;


namespace atp {
namespace couchdb {

struct metadata
{
  string id;
  string rev;
};


class couchdb : NoCopyAndAssign
{
 public:

  typedef int status_code;
  typedef json::Object json_object;
  typedef json::Array json_array;
  typedef json::Value json_value;


  explicit couchdb(const string& host, unsigned int port, const string& db);
  ~couchdb();

  const string& url_prefix() const;


 private:
  class implementation;
  boost::scoped_ptr<implementation> impl_;

};


} // couchdb
} // atp

#endif //ATP_COUCHDB_H_
