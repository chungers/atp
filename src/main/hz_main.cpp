
#include <iostream>
#include <signal.h>
#include <stdio.h>
#include <zmq.hpp>

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/algorithm/string.hpp>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "log_levels.h"
#include "varz/varz.hpp"
#include "zmq/Reactor.hpp"

#include "proto/historian.hpp"
#include "proto/ostream.hpp"

#include "historian/Db.hpp"
#include "historian/DbReactorStrategy.hpp"
#include "historian/time_utils.hpp"

#include "MarketDataSubscriber.hpp"


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

DEFINE_int32(messageBlockSize, 10000, "For periodic output to logs.");

DEFINE_VARZ_int64(subscriber_message_process_micros, 0, "micros in handling message");
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


class DbWriterSubscriber : public atp::MarketDataSubscriber
{
 public :
  DbWriterSubscriber(const boost::shared_ptr<historian::Db>& db,
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
      db_(db)
  {
  }

  bool isReady() {
    return db_->Open();
  }

 protected:
  virtual bool process(const string& topic, const MarketData& marketData)
  {
    boost::uint64_t now = now_micros();
    VARZ_subscriber_messages_received++;
    if (db_->Write(marketData, FLAGS_overwrite)) {

      VARZ_subscriber_messages_persisted++;
      VARZ_subscriber_messages_persisted_marketdata++;

      if (VARZ_subscriber_messages_persisted % FLAGS_messageBlockSize == 0) {
        LOG(INFO) << VARZ_subscriber_messages_persisted << " messages written. "
                  << topic << "=>" << marketData;
      }
    }
    VARZ_subscriber_message_process_micros = now_micros() - now;
    return true;
  }

  virtual bool process(const string& topic, const MarketDepth& marketDepth)
  {
    boost::uint64_t now = now_micros();
    VARZ_subscriber_messages_received++;
    if (db_->Write(marketDepth, FLAGS_overwrite)) {

      VARZ_subscriber_messages_persisted++;
      VARZ_subscriber_messages_persisted_marketdepth++;

      if (VARZ_subscriber_messages_persisted % FLAGS_messageBlockSize == 0) {
        LOG(INFO) << VARZ_subscriber_messages_persisted << " messages written. "
                  << topic << "=>" << marketDepth;
      }
    }
    VARZ_subscriber_message_process_micros = now_micros() - now;
    return true;
  }

 private:
  boost::shared_ptr<historian::Db> db_;
};


const boost::shared_ptr<historian::Db>& GetDbSingleton()
{
  static boost::shared_ptr<historian::Db> DB( new historian::Db(FLAGS_leveldb));
  return DB;
}

void OnTerminate(int param)
{
  LOG(INFO) << "===================== SHUTTING DOWN =======================";

  const boost::shared_ptr<historian::Db> db = GetDbSingleton();
  historian::Db* db_ptr = db.get();
  const string& f = db->GetDbPath();
  if (db_ptr != NULL) {
    delete db_ptr;
    LOG(INFO) << "Closed database " << f;
  }

  LOG(INFO) << "Bye.";
  exit(1);
}


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

  // A single instance of Db is shared across all query handling threads
  // and the subscriber which performs writes.
  // This is because leveldb implements isolation at the leveldb::DB instance
  // leve.  For the readers to see the writes that just committed, they must
  // share the same leveldb::DB instance (hence an instance of historian::Db)
  const boost::shared_ptr<historian::Db>& db = GetDbSingleton();
  if (!db->Open()) {
    LOG(FATAL) << "Cannot open db: " << FLAGS_leveldb;
  }

  historian::DbReactorStrategy strategy(db);

  // continue
  LOG(INFO) << "Starting context.";
  zmq::context_t context(1);

  atp::zmq::Reactor reactor(strategy.socketType(),
                            FLAGS_ep, strategy, &context);

  LOG(INFO) << "Db ready for write.";

  vector<string> subscriptions;
  boost::split(subscriptions, FLAGS_topics, boost::is_any_of(","));

  if (FLAGS_subscribe) {

    DbWriterSubscriber subscriber(db,
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
