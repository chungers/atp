
#include <iostream>
#include <signal.h>
#include <stdio.h>
#include <zmq.hpp>

#include <boost/scoped_ptr.hpp>
#include <boost/algorithm/string.hpp>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "log_levels.h"
#include "marketdata_sink.hpp"
#include "varz/varz.hpp"
#include "zmq/Reactor.hpp"

#include "proto/historian.hpp"

#include "historian/Db.hpp"
#include "historian/DbReactorStrategy.hpp"
#include "historian/time_utils.hpp"


void OnTerminate(int param)
{
  LOG(INFO) << "===================== SHUTTING DOWN =======================";
  LOG(INFO) << "Bye.";
  exit(1);
}

// Flags for reactor / db query processor
DEFINE_string(ep, "tcp://127.0.0.1:1111", "Reactor port");
DEFINE_string(leveldb, "", "Leveldb");

// Flags for subscriber
DEFINE_bool(subscribe, true, "True to turn on subscriber and write to db.");
DEFINE_string(id, "historian-hz", "Id of the subscriber");
DEFINE_string(adminEp, "tcp://127.0.0.1:4444", "Admin endpoint");
DEFINE_string(eventEp, "tcp://127.0.0.1:4445", "Event endpoint");
DEFINE_string(pubsubEp, "tcp://127.0.0.1:7777", "PubSub Event endpoint");
DEFINE_string(topics, "", "Comma-delimited subscription topics");
DEFINE_int32(varz, 18001, "varz server port");
DEFINE_bool(overwrite, true, "True to overwrite db records, false to check.");

DEFINE_VARZ_int64(subscriber_messages_received, 0, "total messages");
DEFINE_VARZ_int64(subscriber_messages_persisted, 0, "total messages persisted");
DEFINE_VARZ_int64(subscriber_messages_persisted_marketdata, 0, "total messages persisted");
DEFINE_VARZ_int64(subscriber_messages_persisted_marketdepth, 0, "total messages persisted");
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


class DbWriterSubscriber : public atp::MarketDataSubscriber
{
 public :
  DbWriterSubscriber(const string& dbfile,
                     const string& id,
                     const string& adminEndpoint,
                     const string& eventEndpoint,
                     const string& endpoint,
                     const vector<string>& subscriptions,
                     int varzPort,
                     ::zmq::context_t* context) :
      atp::MarketDataSubscriber(id, adminEndpoint, eventEndpoint,
                                endpoint, subscriptions,
                                varzPort, context),
      db_(new historian::Db(dbfile))
  {
  }

  bool isReady() {
    return db_->Open();
  }

 protected:
  virtual bool process(const string& topic, const MarketData& marketData)
  {
    VARZ_subscriber_messages_received++;
    if (db_->Write(marketData, FLAGS_overwrite)) {
      VLOG(20) << "Written " << topic << "=>" << marketData;
      VARZ_subscriber_messages_persisted++;
      VARZ_subscriber_messages_persisted_marketdata++;
    }
    return true;
  }

  virtual bool process(const string& topic, const MarketDepth& marketDepth)
  {
    VARZ_subscriber_messages_received++;
    if (db_->Write(marketDepth, FLAGS_overwrite)) {
      VLOG(20) << "Written " << topic << "=>" << marketDepth;
      VARZ_subscriber_messages_persisted++;
      VARZ_subscriber_messages_persisted_marketdepth++;
    }
    return true;
  }

 private:
  boost::scoped_ptr<Db> db_;
};

////////////////////////////////////////////////////////
//
// MAIN
//
int main(int argc, char** argv)
{
  google::SetUsageMessage("ZMQ Reactor for historian db");
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  atp::varz::Varz::initialize();

  VARZ_subscriber_topics = FLAGS_topics;

  // Signal handler: Ctrl-C
  signal(SIGINT, OnTerminate);
  // Signal handler: kill -TERM pid
  void (*terminate)(int);
  terminate = signal(SIGTERM, OnTerminate);
  if (terminate == SIG_IGN) {
    signal(SIGTERM, SIG_IGN);
  }

  historian::DbReactorStrategy strategy(FLAGS_leveldb);
  if (!strategy.OpenDb()) {
    LOG(FATAL) << "Cannot open db: " << FLAGS_leveldb;
  }

  // continue
  LOG(INFO) << "Starting context.";
  zmq::context_t context(1);

  atp::zmq::Reactor reactor(FLAGS_ep, strategy, &context);

  LOG(INFO) << "Db ready for write.";

  vector<string> subscriptions;
  boost::split(subscriptions, FLAGS_topics, boost::is_any_of(","));

  if (FLAGS_subscribe) {

      DbWriterSubscriber subscriber(FLAGS_leveldb,
                                    FLAGS_id, FLAGS_adminEp, FLAGS_eventEp,
                                    FLAGS_pubsubEp, subscriptions,
                                    FLAGS_varz, &context);

      // Open another db connection for writes
      if (!subscriber.isReady()) {
        LOG(FATAL) << "Subscriber not ready!";
      }

      LOG(INFO) << "Handling inbound messages.";
      subscriber.processInbound();

  } else {

    // Block until exit
    reactor.block();

  }
}
