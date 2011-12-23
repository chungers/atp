
#include <signal.h>
#include <gflags/gflags.h>
#include <glog/logging.h>

#include "marketdata.hpp"


void OnTerminate(int param)
{
  LOG(INFO) << "===================== SHUTTING DOWN =======================";
  LOG(INFO) << "Bye.";
  exit(1);
}

DEFINE_string(ep, "tcp://127.0.0.1:7777", "Marketdata endpoint");
DEFINE_string(topic, "", "Subscription topic");

using namespace std;

class ConsoleMarketDataSubscriber : public atp::MarketDataSubscriber
{
 public :
  ConsoleMarketDataSubscriber(const string& endpoint,
                              const string& subscription,
                              ::zmq::context_t* context) :
      MarketDataSubscriber(endpoint, subscription, context)
  {
  }

 protected:
  virtual bool process(const boost::posix_time::ptime& ts, const string& topic,
                       const string& key, const string& value)
  {
    using namespace boost::posix_time;
    ptime now = microsec_clock::universal_time();
    time_duration latency = now - ts;

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

  ::ConsoleMarketDataSubscriber subscriber(FLAGS_ep, FLAGS_topic, &context);

  LOG(INFO) << "Start handling inbound messages.";
  subscriber.processInbound();

}


