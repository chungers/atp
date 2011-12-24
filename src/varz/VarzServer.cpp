#include <iostream>
#include <sstream>
#include <map>
#include <stdio.h>
#include <string.h>
#include <vector>

#include <boost/thread.hpp>

extern "C" {
#include "mongoose/mongoose.h"
}

#include "log_levels.h"
#include "varz/varz.hpp"
#include "varz/VarzServer.hpp"


namespace atp {
namespace varz {

namespace internal {


static void get_qsvar(const struct mg_request_info *request_info,
                      const char *name, char *dst, size_t dst_len) {
  const char *qs = request_info->query_string;
  mg_get_var(qs, strlen(qs == NULL ? "" : qs), name, dst, dst_len);
}


static const char * HTTP_200 =
  "HTTP/1.1 200 OK\r\n"
  "Cache: no-cache\r\n"
  "Content-Type: application/json; charset=utf-8\r\n"
  "\r\n";

static void *event_handler(enum mg_event event,
                           struct mg_connection *conn,
                           const struct mg_request_info *request_info) {
  using namespace std;
  using namespace atp::varz;

  char buff[] = "yes";
  void *processed = reinterpret_cast<void *>(buff);

  VARZ_LOGGER << "Request uri = " << request_info->uri
              << " query string = " << request_info->query_string;

  if (event == MG_NEW_REQUEST) {

    if (strcmp(request_info->uri, "/echo") == 0) {
      char message[256];
      // Fetch parameter
      get_qsvar(request_info, "message", message, sizeof(message));

      VARZ_DEBUG << "Echo request: " << message;

      mg_printf(conn, "%s", HTTP_200);
      mg_printf(conn, "{ \"message\" : \"%s\" }\n", message);

    } else if (strcmp(request_info->uri, "/varz") == 0) {

      VARZ_DEBUG << "varz request";
      mg_printf(conn, "%s", HTTP_200);
      mg_printf(conn, "%s", "{");

      vector<VarzInfo> varzs;
      GetAllVarzs(&varzs);
      int i = 0;
      for (vector<VarzInfo>::iterator varz = varzs.begin();
           varz != varzs.end();
           ++varz) {
        mg_printf(conn, "%s\"%s\" : \"%s\"",
                  ((i++ == 0) ? "\n" : ",\n"),
                  varz->name.c_str(), varz->current_value.c_str());
      }

      mg_printf(conn, "\n%s\n", "}");

    } else {
      // No suitable handler found, mark as not processed. Mongoose will
      // try to serve the request.
      processed = NULL;
    }

  } else {
    processed = NULL;
  }

  return processed;
}

using namespace std;

/// Wrapper for Mongoose config, which represents options as
/// char** of C strings.
class MongooseConfig
{
 public:
  MongooseConfig() {
  }

  void setPort(int port)
  {
    configs.push_back("listening_ports");
    ostringstream p;
    p << port;
    configs.push_back(p.str());
  }

  void setThreads(int threads)
  {
    configs.push_back("num_threads");
    ostringstream t;
    t << threads;
    configs.push_back(t.str());
  }

  operator const char **()
  {
    ptrs.resize(configs.size() + 1);
    long i= configs.size();
    ptrs[i] = 0;
    for (--i; i >= 0; --i) {
      ptrs[i] = configs[i].c_str();
    }
    return &(ptrs[0]);
  }

 private:
  vector<string> configs;
  vector<const char*> ptrs;
};

} // namespace internal


class VarzServer::implementation
{
 public:
  implementation(int port, int num_threads) :
      port_(port),
      numThreads_(num_threads),
      serverContext_(NULL),
      running_(false)
  {
  }

  ~implementation()
  {
    stop();
  }

  void start()
  {
    boost::unique_lock<boost::mutex> lock(mutex_);

    if (serverContext_ == NULL && !running_) {
      using namespace atp::varz::internal;
      using namespace std;

      MongooseConfig config;
      config.setPort(port_);
      config.setThreads(numThreads_);
      serverContext_ = mg_start(&event_handler, NULL, config);

      VARZ_LOGGER << "Varz server listening on " << port_
                  << " with " << numThreads_ << " threads.";

      running_ = true;
    }
  }

  void stop()
  {
    boost::unique_lock<boost::mutex> lock(mutex_);

    if (serverContext_ != NULL && running_) {
      VARZ_LOGGER << "Stopping context.";
      mg_stop(serverContext_);
      running_ = false;
    }
  }

 private:
  int port_;
  int numThreads_;
  struct mg_context* serverContext_;
  bool running_;
  boost::mutex mutex_;
};

VarzServer::VarzServer(int port, int num_threads) :
    impl_(new implementation(port, num_threads))
{
}

VarzServer::~VarzServer()
{
}

void VarzServer::start()
{
  impl_->start();
}

void VarzServer::stop()
{
  impl_->stop();
}

} // namespace varz
} // namespace atp



