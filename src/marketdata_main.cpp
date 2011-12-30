
#include <vector>
#include <signal.h>
#include <gflags/gflags.h>
#include <glog/logging.h>

#include <boost/algorithm/string.hpp>

#include "varz/varz.hpp"
#include "varz/VarzServer.hpp"

#include "marketdata_sink.hpp"

static atp::varz::VarzServer* varz_instance;

void OnTerminate(int param)
{
  LOG(INFO) << "===================== SHUTTING DOWN =======================";
  if (varz_instance != NULL) {
    varz_instance->stop();
    LOG(INFO) << "Stopped varz";
  }
  LOG(INFO) << "Bye.";
  exit(1);
}

DEFINE_string(ep, "tcp://127.0.0.1:7777", "Marketdata endpoint");
DEFINE_string(topics, "", "Commad delimited subscription topics");
DEFINE_bool(playback, false, "True if data is playback from logs");
DEFINE_int32(varz, 9999, "varz server port");

DEFINE_VARZ_int64(subscriber_messages_received, 0, "total messages");
DEFINE_VARZ_bool(subscriber_latency_offset, false, "latency offset");
DEFINE_VARZ_string(subscriber_topics, "", "subscriber topics");


using namespace std;

class ConsoleMarketDataSubscriber : public atp::MarketDataSubscriber
{
 public :
  ConsoleMarketDataSubscriber(const string& endpoint,
                              const vector<string>& subscriptions,
                              ::zmq::context_t* context) :
      MarketDataSubscriber(endpoint, subscriptions, context)
  {
  }

 protected:
  virtual bool process(const boost::posix_time::ptime& ts, const string& topic,
                       const string& key, const string& value,
                       const boost::posix_time::time_duration& latency)
  {
    VARZ_subscriber_messages_received++;

    LOG(INFO) << topic << ' '
              << us_eastern::utc_to_local(ts)
              << ' ' << key << ' ' << value << ' ' << latency;
    return true;
  }

};

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

  ::ConsoleMarketDataSubscriber subscriber(FLAGS_ep, subscriptions, &context);
  subscriber.setOffsetLatency(FLAGS_playback);

  LOG(INFO) << "Start varz server at " << FLAGS_varz;
  atp::varz::VarzServer varz(FLAGS_varz, 2);
  varz_instance = &varz;
  varz.start();

  LOG(INFO) << "Start handling inbound messages.";
  subscriber.processInbound();

}


