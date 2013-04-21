
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


namespace internal {

/// callback function used by curl to get the response data.
static size_t data_write(void* buf, size_t size, size_t nmemb, void* userp)
{
  if(userp) {
    std::ostream& os = *static_cast<std::ostream*>(userp);
    std::streamsize len = size * nmemb;
    if(os.write(static_cast<char*>(buf), len))
      return len;
  }
  return 0;
}


}

class couchdb::implementation
{
 public:
  explicit implementation(const string& host,
                          unsigned int port,
                          const string& db,
                          long timeout = 1000) :
      host_(host), port_(port), db_(db), timeout_(timeout)
  {
    std::ostringstream oss;
    oss << "http://" << host_ << ':' << port_ << '/' << db;
    url_prefix_ = oss.str();
  }

  const string& url_prefix() const
  {
    return url_prefix_;
  }

  couchdb::raw_response put(const string& key,
                            const couchdb::json_string& json)
  {
    CURL* curl = curl_easy_init();

    couchdb::status_code http_code(couchdb::INIT_FAILED);

    if (curl) {

      std::ostringstream url, curl_response;
      url << url_prefix() << '/' << key;

      curl_easy_setopt(curl, CURLOPT_URL, url.str().c_str());
      curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json.c_str());
      curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
      curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout_);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &internal::data_write);
      curl_easy_setopt(curl, CURLOPT_FILE, &curl_response);

      CURLcode res = curl_easy_perform(curl);
      if (res == 0) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
      }
      curl_easy_cleanup(curl);
      return couchdb::raw_response(http_code, curl_response.str());

    } else {
      return couchdb::raw_response(http_code,
                                   "{\"message\":\"Cannot initialize curl\"}");
    }
  }

  couchdb::raw_response get(const string& key) const
  {
    CURL* curl = curl_easy_init();

    couchdb::status_code http_code(couchdb::INIT_FAILED);

    if (curl) {

      std::ostringstream url, curl_response;
      url << url_prefix() << '/' << key;

      curl_easy_setopt(curl, CURLOPT_URL, url.str().c_str());
      curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
      curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
      curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout_);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &internal::data_write);
      curl_easy_setopt(curl, CURLOPT_FILE, &curl_response);

      CURLcode res = curl_easy_perform(curl);
      if (res == 0) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
      }
      curl_easy_cleanup(curl);
      return couchdb::raw_response(http_code, curl_response.str());

    } else {
      return couchdb::raw_response(http_code,
                                   "{\"message\":\"Cannot initialize curl\"}");
    }
  }

 private:
  const string host_;
  const unsigned int port_;
  const string db_;
  const long timeout_;

  string url_prefix_;
};


const couchdb::status_code couchdb::INIT_FAILED = 599;
const couchdb::status_code couchdb::NEW = 201;
const couchdb::status_code couchdb::GET_OK = 200;

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

couchdb::raw_response couchdb::put(const string& key,
                                   const json_string& json)
{
  return impl_->put(key, json);
}

couchdb::raw_response couchdb::get(const string& key) const
{
  return impl_->get(key);
}

} // couchdb
} // atp
