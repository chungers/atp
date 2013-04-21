#ifndef ATP_COUCHDB_H_
#define ATP_COUCHDB_H_

#include <map>
#include <string>
#include <boost/scoped_ptr.hpp>
#include <json_spirit.h>

#include "common.hpp"


using std::string;
using std::pair;
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
  typedef string json_string;
  typedef string doc_rev;

  typedef pair<status_code, json_string> raw_response;


  typedef json::Object json_object;
  typedef json::Array json_array;
  typedef json::Value json_value;

  const static status_code INIT_FAILED;
  const static status_code NEW;
  const static status_code GET_OK;

  explicit couchdb(const string& host, unsigned int port, const string& db);
  ~couchdb();

  const string& url_prefix() const;

  raw_response put(const string& key, const json_string& json);

  raw_response get(const string& key) const;

 private:
  class implementation;
  boost::scoped_ptr<implementation> impl_;

};



} // couchdb
} // atp

#endif //ATP_COUCHDB_H_
