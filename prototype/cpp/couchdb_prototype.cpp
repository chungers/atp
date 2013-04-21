

#include <string>
#include <iostream>
#include <math.h>
#include <vector>

#include <curl/curl.h>

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <boost/thread.hpp>

#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include <glog/logging.h>

#include "common/time_utils.hpp"


using std::string;
using std::ostringstream;

const string COUCHDB("http://127.0.0.1:5984/dev");

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


TEST(CouchDBPrototype, DocCreation)
{
  LOG(INFO) << "Initializaing curl";

  CURL* curl = curl_easy_init();

  LOG(INFO) << "Curl initialized.";

  int http_code;
  int docs = 10;
  long timeout = 1000;

  // ostringstream response;

  for (int i = 0; i < docs; ++i) {

    boost::uint64_t ts = now_micros();

    ostringstream urls;
    urls << COUCHDB << '/' << "test-" << ts;

    ostringstream oss;
    oss << "{\"id\":\"" << ts << "\"}";

    string url(urls.str());
    string data(oss.str());

    if (curl) {

      try {

        LOG(INFO) << "About to create doc " << url;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout);

        CURLcode res = curl_easy_perform(curl);


        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        LOG(INFO) << "Got result " << res << ", http status = " << http_code;

        LOG(INFO) << "Now trying to read " << url;

        ostringstream response;
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &data_write);
        curl_easy_setopt(curl, CURLOPT_FILE, &response);

        res = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        LOG(INFO) << "Read result " << res << ", http status = " << http_code
                  << ", data = " << response.str();

      } catch (...) {
        LOG(ERROR) << "error";
      }
    }
  }

  if (curl) {
    curl_easy_cleanup(curl);
  }
}


