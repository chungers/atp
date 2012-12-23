/// Simple main to set up market data subscription

#include <iostream>
#include <vector>
#include <signal.h>
#include <gflags/gflags.h>
#include <glog/logging.h>

#include <boost/algorithm/string.hpp>

#include "common/time_utils.hpp"
#include "platform/marketdata_handler_proto_impl.hpp"
#include "platform/message_processor.hpp"

#include "main/global_defaults.hpp"



DEFINE_string(fhPublishEp, atp::global::FH_OUTBOUND_ENDPOINT,
              "FH market data publish endpoint");
DEFINE_string(symbols, "",
              "Comma-delimited list of stocks to watch.");

void OnTerminate(int param)
{
  LOG(INFO) << "===================== SHUTTING DOWN =======================";
  LOG(INFO) << "Bye.";
  exit(1);
}

using std::string;
using std::vector;
namespace platform = atp::platform;
namespace p = proto::ib;

struct state_t
{
  state_t(const string& symbol) :
      symbol(symbol), bid(0.), ask(0.), last(0.),
      bid_size(0), ask_size(0), last_size(0) {}

  string symbol;
  double bid, ask, last;
  int bid_size, ask_size, last_size;
  boost::posix_time::ptime ct;
};

template <typename V>
void print(const timestamp_t& ts, const V& v,
           V* state_var, state_t* state)
{
  boost::posix_time::ptime t = atp::time::as_ptime(ts);
  state->ct = t;
  *state_var = v;

  std::cout << atp::time::to_est(t) << ',' << state->symbol << ','
            << "bid=" << state->bid << '/' << state->bid_size << ','
            << "ask=" << state->ask << '/' << state->ask_size << ','
            << "trade=" << state->last << '/' << state->last_size;

  if (state->last > state->ask) {
    std::cout << " VVV";
  } else if (state->last < state->bid) {
    std::cout << " ^^^";
  } else if (state->bid > state->ask) {
    std::cout << " !!!";
  }
  std::cout << std::endl;
}


bool stop_function(const string& topic, const string& message)
{
  LOG(INFO) << "Stopping: " << topic;
  return false;
}


int main(int argc, char** argv)
{
  google::SetUsageMessage("Marketadata watcher");
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

  // map of handlers for all symbols
  platform::message_processor::protobuf_handlers_map symbols_map;

  // symbols are in the form of the security key, e.g. APPL.STK
  vector<string> keys;
  boost::split(keys, FLAGS_symbols, boost::is_any_of(","));

  int request = 0;
  for (vector<string>::const_iterator itr = keys.begin();
       itr != keys.end(); ++itr, ++request) {

    state_t* state = new state_t(*itr);

    platform::marketdata::marketdata_handler<p::MarketData> *handler = new
        platform::marketdata::marketdata_handler<p::MarketData>();

    platform::callback::update_event<double>::func bid =
        boost::bind(&print<double>, _1, _2, &(state->bid), state);

    platform::callback::update_event<double>::func ask =
        boost::bind(&print<double>, _1, _2, &(state->ask), state);

    platform::callback::update_event<double>::func last =
        boost::bind(&print<double>, _1, _2, &(state->last), state);

    platform::callback::update_event<int>::func bid_size =
        boost::bind(&print<int>, _1, _2, &(state->bid_size), state);

    platform::callback::update_event<int>::func ask_size =
        boost::bind(&print<int>, _1, _2, &(state->ask_size), state);

    platform::callback::update_event<int>::func last_size =
        boost::bind(&print<int>, _1, _2, &(state->last_size), state);

    handler->bind("BID", bid);
    handler->bind("ASK", ask);
    handler->bind("LAST", last);
    handler->bind("BID_SIZE", bid_size);
    handler->bind("ASK_SIZE", ask_size);
    handler->bind("LAST_SIZE", last_size);

    symbols_map.register_handler(state->symbol, *handler);
    symbols_map.register_handler("STOP",
                         boost::bind(&stop_function, _1, _2));
  }

  platform::message_processor agent(FLAGS_fhPublishEp, symbols_map);

  agent.block();
}
