
#include <iostream>
#include <vector>
#include <signal.h>
#include <gflags/gflags.h>
#include <glog/logging.h>

#include <boost/algorithm/string.hpp>

#include "varz/varz.hpp"

#include "marketdata_sink.hpp"
#include "historian/time_utils.hpp"


DEFINE_string(adminEp, "tcp://127.0.0.1:4444", "Admin endpoint");
DEFINE_string(eventEp, "tcp://127.0.0.1:4445", "Event endpoint");
DEFINE_string(id, "marketdata", "Id of the subscriber");
DEFINE_string(ep, "tcp://127.0.0.1:7777", "Marketdata endpoint");
DEFINE_string(topics, "", "Comma-delimited subscription topics");
DEFINE_bool(playback, false, "True if data is playback from logs");
DEFINE_int32(varz, 18002, "varz server port");

DEFINE_VARZ_int64(subscriber_messages_received, 0, "total messages");
DEFINE_VARZ_bool(subscriber_latency_offset, false, "latency offset");
DEFINE_VARZ_string(subscriber_topics, "", "subscriber topics");


using namespace std;
using namespace historian;

using boost::posix_time::ptime;
using boost::posix_time::time_duration;
using proto::common::Value;
using proto::ib::MarketData;
using proto::ib::MarketDepth;

std::ostream& operator<<(std::ostream& out, const Value& v)
{
  using namespace proto::common;
  switch (v.type()) {
    case Value_Type_INT:
      out << v.int_value();
      break;
    case Value_Type_DOUBLE:
      out << v.double_value();
      break;
    case Value_Type_STRING:
      out << v.string_value();
      break;
  }
  return out;
}

std::ostream& operator<<(std::ostream& out, const MarketData& v)
{
  ptime t =to_est(as_ptime(v.timestamp()));
  out << t << "," << v.symbol() << "," << v.event() << "," << v.value();
  return out;
}

std::ostream& operator<<(std::ostream& out, const MarketDepth& v)
{
  ptime t = to_est(as_ptime(v.timestamp()));
  out << t << "," << v.symbol() << ","
      << v.operation() << "," << v.level() << ","
      << v.side() << "," << v.price() << ","
      << v.size();
  return out;
}


class ConsoleMarketDataSubscriber : public atp::MarketDataSubscriber
{
 public :
  ConsoleMarketDataSubscriber(const string& id,
                              const string& adminEndpoint,
                              const string& eventEndpoint,
                              const string& endpoint,
                              const vector<string>& subscriptions,
                              int varzPort,
                              ::zmq::context_t* context) :
      MarketDataSubscriber(id, adminEndpoint, eventEndpoint,
                           endpoint, subscriptions,
                           varzPort, context)
  {
  }

 protected:
  virtual bool process(const string& topic, const MarketData& marketData)
  {
    VARZ_subscriber_messages_received++;

    LOG(INFO) << topic << ' ' << marketData;
    return true;
  }

  virtual bool process(const string& topic, const MarketDepth& marketDepth)
  {
    VARZ_subscriber_messages_received++;

    LOG(INFO) << topic << ' ' << marketDepth;
    return true;
  }
};



void OnTerminate(int param)
{
  LOG(INFO) << "===================== SHUTTING DOWN =======================";
  LOG(INFO) << "Bye.";
  exit(1);
}


int main(int argc, char** argv)
{
  google::SetUsageMessage("Marketdata Subscriber");
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  atp::varz::Varz::initialize();

  VARZ_subscriber_topics = FLAGS_topics;
  VARZ_subscriber_latency_offset = FLAGS_playback;

  // Signal handler: Ctrl-C
  signal(SIGINT, OnTerminate);
  // Signal handler: kill -TERM pid
  void (*terminate)(int);
  terminate = signal(SIGTERM, OnTerminate);
  if (terminate == SIG_IGN) {
    signal(SIGTERM, SIG_IGN);
  }

  LOG(INFO) << "Starting context.";
  zmq::context_t context(1);

  vector<string> subscriptions;
  boost::split(subscriptions, FLAGS_topics, boost::is_any_of(","));

  ::ConsoleMarketDataSubscriber subscriber(
      FLAGS_id, FLAGS_adminEp, FLAGS_eventEp,
      FLAGS_ep, subscriptions, FLAGS_varz, &context);
  subscriber.setOffsetLatency(FLAGS_playback);

  LOG(INFO) << "Start handling inbound messages.";
  subscriber.processInbound();

}


