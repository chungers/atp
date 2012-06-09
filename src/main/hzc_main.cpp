

#include <signal.h>
#include <stdio.h>
#include <vector>

#include <zmq.hpp>

#include <boost/optional.hpp>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "log_levels.h"

#include "proto/historian.hpp"
#include "proto/ostream.hpp"

#include "historian/time_utils.hpp"
#include "historian/Visitor.hpp"
#include "historian/DbReactorClient.hpp"


void OnTerminate(int param)
{
  LOG(INFO) << "===================== SHUTTING DOWN =======================";
  LOG(INFO) << "Bye.";
  exit(1);
}

DEFINE_bool(stream, true, "True to stream, false to buffer in array.");
DEFINE_bool(cout, true, "Send output to cout");
DEFINE_string(ep, "tcp://127.0.0.1:1111", "Reactor port");
DEFINE_string(cb, "tcp://127.0.0.1:1112", "Callback port");
DEFINE_string(symbol, "", "Symbol");
DEFINE_string(first, "", "First of range");
DEFINE_string(last, "", "Last of range");
DEFINE_string(event, "", "Event (e.g. BID, ASK)");


////////////////////////////////////////////////////////
//
// MAIN
//
using std::string;
using zmq::context_t;

using boost::posix_time::ptime;
using boost::optional;

using proto::ib::MarketData;
using proto::ib::MarketDepth;
using proto::historian::SessionLog;
using proto::historian::IndexedValue;
using proto::historian::QueryBySymbol;

using namespace historian;
using namespace proto::historian;


int main(int argc, char** argv)
{
  google::SetUsageMessage("ZMQ Reactor for historian / marketdata db");
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  // Signal handler: Ctrl-C
  signal(SIGINT, OnTerminate);
  // Signal handler: kill -TERM pid
  void (*terminate)(int);
  terminate = signal(SIGTERM, OnTerminate);
  if (terminate == SIG_IGN) {
    signal(SIGTERM, SIG_IGN);
  }

  context_t context(1);
  historian::DbReactorClient client(FLAGS_ep, FLAGS_cb, &context);

  if (!client.connect()) {
    LOG(FATAL) << "Cannot connecto to " << FLAGS_ep;
  }

  HISTORIAN_REACTOR_LOGGER << "Connected to " << FLAGS_ep;

  ptime start;
  ptime end;

  // EST to UTC
  historian::parse(FLAGS_first, &start);
  historian::parse(FLAGS_last, &end);

  QueryBySymbol q;
  q.set_symbol(FLAGS_symbol);
  q.set_utc_first_micros(historian::as_micros(start));
  q.set_utc_last_micros(historian::as_micros(end));

  // TODO abstract this detail into a cleaner api
  if (FLAGS_event.size() > 0) {
    q.set_type(proto::historian::INDEXED_VALUE);
    q.set_index(FLAGS_event);
  }


  struct Visitor : public historian::Visitor {

    virtual bool operator()(const Record& data)
    {
      using namespace proto::common;
      using namespace proto::historian;

      optional<IndexedValue> v1 = as<IndexedValue>(data);
      if (v1) {
        if (FLAGS_stream) {
          if (FLAGS_cout) std::cout << data.key() << "," << *v1;
        } else {
          indexedValues.push_back(*v1);
        }
      }
      optional<MarketData> v2 = as<MarketData>(data);
      if (v2) {
        if (FLAGS_stream) {
          if (FLAGS_cout) std::cout << data.key() << "," << *v2;
        } else {
          marketData.push_back(*v2);
        }
      }
      if (FLAGS_stream) {
        if (FLAGS_cout) std::cout << std::endl;
      }
      return true;
    }

    std::vector<IndexedValue> indexedValues;
    std::vector<MarketData> marketData;

  } visitor;

  LOG(INFO) << "Query: " << q;

  boost::uint64_t start_micros = now_micros();
  int count = client.Query(q, &visitor);
  boost::uint64_t finish_micros = now_micros();

  double totalSeconds = static_cast<double>(finish_micros - start_micros)
      / 1000000.;
  double qps = static_cast<double>(count) / totalSeconds;

  LOG(INFO) << "Count = " << count << " in "
            << totalSeconds << ", qps = " << qps;


  if (!FLAGS_stream) {
    if (visitor.indexedValues.size() > 0) {
      std::vector<IndexedValue>::const_iterator itr =
          visitor.indexedValues.begin();
      for (; itr != visitor.indexedValues.end(); ++itr) {
        if (FLAGS_cout) std::cout << *itr << std::endl;
      }
    }
    if (visitor.marketData.size() > 0) {
      std::vector<MarketData>::const_iterator itr =
          visitor.marketData.begin();
      for (; itr != visitor.marketData.end(); ++itr) {
        if (FLAGS_cout) std::cout << *itr << std::endl;
      }
    }
  }

  LOG(INFO) << "Count = " << count << " in "
            << totalSeconds << ", qps = " << qps;

}
