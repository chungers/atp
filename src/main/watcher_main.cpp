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



DEFINE_string(fhHost, atp::global::FH_HOST,
              "FH market data publish host");
DEFINE_int32(fhPort, atp::global::FH_OUTBOUND_PORT,
              "FH market data publish port");
DEFINE_string(symbols, "",
              "Comma-delimited list of stocks to watch.");
DEFINE_bool(last, false,
            "Update only on last trade");


#define CONSOLE_RED "\033[1;31m"
#define CONSOLE_GREEN "\033[1;32m"
#define CONSOLE_YELLOW "\033[1;33m"
#define CONSOLE_BLUE "\033[1;34m"
#define CONSOLE_MAGENTA "\033[1;35m"
#define CONSOLE_CYAN "\033[1;36m"
#define CONSOLE_WHITE "\033[1;37m"
#define CONSOLE_RESET "\033[1;0m"
#define CONSOLE_BOLD "\033[1;1m"
#define CONSOLE_UNDERLINE "\033[1;4m"
#define CONSOLE_BLINK "\033[1;5m"
#define CONSOLE_REVERSE "\033[1;7m"


using std::string;
using std::vector;
namespace platform = atp::platform;
namespace p = proto::ib;

struct state_t
{
  state_t(const string& symbol) :
      symbol(symbol), bid(0.), ask(0.), last(0.), mid(0.), spread(0.),
      bid_size(0), ask_size(0), last_size(0),
      balance(0.)
  {}

  string symbol;
  double bid, ask, last, mid, spread;
  int bid_size, ask_size, last_size;
  double balance;
  boost::posix_time::ptime ct;
};

void OnTerminate(int param)
{
  std::cout << CONSOLE_RESET << std::endl;
  LOG(INFO) << "===================== SHUTTING DOWN =======================";
  LOG(INFO) << "Bye.";
  exit(1);
}

template <typename V>
void print(const timestamp_t& ts, const V& v,
           V* state_var, state_t* state, bool print)
{
  if (v == 0) return;

  boost::posix_time::ptime t = atp::time::as_ptime(ts);
  state->ct = t;
  *state_var = v;

  state->spread = state->ask - state->bid;

  // compute balance (from jbooktrader: http://goo.gl/eqykk)
  state->mid = (state->bid + state->ask) / 2.;
  double total_top = state->bid_size + state->ask_size;
  if (total_top != 0.) {
    state->balance = 100. * (state->bid_size - state->ask_size) / total_top;
  } else {
    state->balance = 0.;
  }

  // print output
  if (!print) return;

  string color;
  if (state->last >= state->ask) {
    color = CONSOLE_GREEN;
    if (state->last == state->ask) {
      color += CONSOLE_UNDERLINE;
    }
  } else if (state->last <= state->bid) {
    color = CONSOLE_RED;
    if (state->last == state->bid) {
      color += CONSOLE_UNDERLINE;
    }
  } else if (state->last > state->mid) {
    color = CONSOLE_CYAN;
  } else if (state->last == state->mid) {
    color = CONSOLE_WHITE;
  } else if (state->last < state->mid) {
    color = CONSOLE_YELLOW;
  } else if (state->bid > state->ask) {
    color = CONSOLE_MAGENTA;
  } else {
    color = CONSOLE_RESET;
  }

  std::cout << color
            << atp::time::to_est(t) << ',' << state->symbol << ','
            << state->bid << '/' << state->bid_size << ','
            << state->mid << ','
            << state->ask << '/' << state->ask_size << ','
            << state->spread << ','
            << state->balance << ','
            << state->last << '/' << state->last_size;

  std::cout << CONSOLE_RESET << std::endl;
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

  std::string zmqVersion;
  atp::zmq::version(&zmqVersion);

  LOG(INFO) << "ZMQ " << zmqVersion;

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
        boost::bind(&print<double>, _1, _2, &(state->bid), state, !FLAGS_last);

    platform::callback::update_event<double>::func ask =
        boost::bind(&print<double>, _1, _2, &(state->ask), state, !FLAGS_last);

    platform::callback::update_event<double>::func last =
        boost::bind(&print<double>, _1, _2, &(state->last), state, !FLAGS_last);

    platform::callback::update_event<int>::func bid_size =
        boost::bind(&print<int>, _1, _2, &(state->bid_size), state, !FLAGS_last);

    platform::callback::update_event<int>::func ask_size =
        boost::bind(&print<int>, _1, _2, &(state->ask_size), state, !FLAGS_last);

    platform::callback::update_event<int>::func last_size =
        boost::bind(&print<int>, _1, _2, &(state->last_size), state, true);

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

  platform::message_processor agent(atp::zmq::EndPoint::tcp(
      FLAGS_fhPort, FLAGS_fhHost), symbols_map);

  agent.block();
}
