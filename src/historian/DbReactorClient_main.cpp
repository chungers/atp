

#include <signal.h>
#include <stdio.h>
#include <zmq.hpp>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "log_levels.h"

#include "historian/time_utils.hpp"
#include "historian/Visitor.hpp"
#include "historian/DbReactorClient.hpp"


void OnTerminate(int param)
{
  LOG(INFO) << "===================== SHUTTING DOWN =======================";
  LOG(INFO) << "Bye.";
  exit(1);
}


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

using proto::ib::MarketData;
using proto::ib::MarketDepth;
using proto::historian::SessionLog;
using proto::historian::QueryByRange;

using namespace historian;


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

  if (FLAGS_symbol.size() > 0) {

  } else {

    QueryByRange q;
    q.set_first(FLAGS_first);
    q.set_last(FLAGS_last);

    // TODO abstract this detail into a cleaner api
    q.set_type(proto::historian::VALUE);
    if (FLAGS_event.size() > 0) {
      q.set_index(INDEX_IB_MARKET_DATA_BY_EVENT);
    }

    struct Visitor : public historian::DefaultVisitor {

      virtual bool operator()(const MarketData& data)
      {
        std::cerr << "marketdata: " << data.symbol()
                  << ", " << data.timestamp() << ", " << data.event()
                  << std::endl;
        return true;
      }
    } visitor;

    int count = client.query(q, &visitor);
    HISTORIAN_REACTOR_LOGGER << "Count = " << count;
  }
}
