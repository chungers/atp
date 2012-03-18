#ifndef ATP_MARKETDATA_SINK_H_
#define ATP_MARKETDATA_SINK_H_

#include <map>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/assign.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/cstdint.hpp>
#include <boost/date_time/gregorian/greg_month.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <boost/thread.hpp>

#include "log_levels.h"
#include "managed_agent.hpp"
#include "utils.hpp"
#include "historian/constants.hpp"
#include "proto/historian.hpp"
#include "varz/varz.hpp"
#include "zmq/ZmqUtils.hpp"


DEFINE_VARZ_int64(marketdata_process_latency_micros, 0, "");
DEFINE_VARZ_int64(marketdata_process_latency_micros_total, 0, "");
DEFINE_VARZ_int64(marketdata_process_latency_micros_count, 0, "");
DEFINE_VARZ_int64(marketdata_process_latency_over_budget, 0, "");
DEFINE_VARZ_int64(marketdata_process_latency_drift_micros, 0, "");

DEFINE_VARZ_bool(marketdata_process_stopped, false, "");

DEFINE_VARZ_string(marketdata_id, "", "");
DEFINE_VARZ_string(marketdata_subscribes, "", "");
DEFINE_VARZ_string(marketdata_unsubscribes, "", "");

DEFINE_VARZ_int64(marketdata_event_last_ts, 0, "");
DEFINE_VARZ_int64(marketdata_event_interval_micros, 0, "");

DEFINE_VARZ_int64(marketdepth_process_latency_micros, 0, "");
DEFINE_VARZ_int64(marketdepth_process_latency_micros_total, 0, "");
DEFINE_VARZ_int64(marketdepth_process_latency_micros_count, 0, "");
DEFINE_VARZ_int64(marketdepth_process_latency_over_budget, 0, "");
DEFINE_VARZ_int64(marketdepth_process_latency_drift_micros, 0, "");

DEFINE_VARZ_bool(marketdepth_process_stopped, false, "");

DEFINE_VARZ_int64(marketdepth_event_last_ts, 0, "");
DEFINE_VARZ_int64(marketdepth_event_interval_micros, 0, "");

namespace atp {

using namespace std;


using boost::posix_time::time_duration;
using proto::ib::MarketData;
using proto::ib::MarketDepth;


class MarketDataSubscriber : public ManagedAgent
{
 public:

  MarketDataSubscriber(const string& id,
                       const string& adminEndpoint,
                       const string& eventEndpoint,
                       const string& endpoint,
                       const vector<string>& subscriptions,
                       int varzPort = 18000,
                       ::zmq::context_t* context = NULL) :
      ManagedAgent(id, adminEndpoint, eventEndpoint, varzPort),
      endpoint_(endpoint),
      subscriptions_(subscriptions),
      contextPtr_(context),
      ownContext_(context == NULL),
      offsetLatency_(false)
  {
    VARZ_marketdata_id = getId();

    // Bind admin handlers
    registerHandler("connect", boost::bind(
        &MarketDataSubscriber::connect, this, _2));
    registerHandler("subscribe", boost::bind(
        &MarketDataSubscriber::subscribe, this, _2));
    registerHandler("unsubscribe", boost::bind(
        &MarketDataSubscriber::unsubscribe, this, _2));
    registerHandler("stop", boost::bind(
        &MarketDataSubscriber::stop, this));

    if (ownContext_) {
      contextPtr_ = new ::zmq::context_t(1);
    }
    socketPtr_ = new ::zmq::socket_t(*contextPtr_, ZMQ_SUB);
    socketPtr_->connect(endpoint_.c_str());
    // Set subscriptions
    for (vector<string>::iterator sub = subscriptions_.begin();
         sub != subscriptions_.end();
         ++sub) {
      subscribe(*sub);
      MARKET_DATA_SUBSCRIBER_LOGGER << "subscribed to topic = " << *sub;
    }

    ManagedAgent::initialize();
  }

  ~MarketDataSubscriber()
  {
    if (socketPtr_ != NULL) {
      delete socketPtr_;
    }
    if (ownContext_) {
      delete contextPtr_;
    }
  }

  virtual bool stop()
  {
    MARKET_DATA_SUBSCRIBER_LOGGER <<
        "Stopping marketdata subscription processing.";
    return false;
  }

  /// Connect to another endpoint IN ADDITION to the current connection(s).
  bool connect(const string& endpoint)
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    if (socketPtr_ != NULL) {
      socketPtr_->connect(endpoint.c_str());
      return true;
    }
    return false;
  }

  bool subscribe(const string& topic)
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    if (socketPtr_ != NULL) {
      socketPtr_->setsockopt(ZMQ_SUBSCRIBE,
                             topic.c_str(), topic.length());
      VARZ_marketdata_subscribes += topic;
      VARZ_marketdata_subscribes += " ";
      return true;
    }
    return false;
  }

  bool unsubscribe(const string& topic)
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    if (socketPtr_ != NULL) {
      socketPtr_->setsockopt(ZMQ_UNSUBSCRIBE,
                             topic.c_str(), topic.length());
      VARZ_marketdata_unsubscribes += topic;
      VARZ_marketdata_unsubscribes += " ";
      return true;
    }
    return false;
  }

  void setOffsetLatency(bool value)
  {
    offsetLatency_ = value;
  }

  // The event loop.
  void processInbound()
  {
    using namespace boost::posix_time;
    using namespace historian;
    using namespace proto::ib;
    using boost::uint64_t;


    long count = 0;
    time_duration latencyOffset;

    while (1) {

      string frame1, frame2, frame3; // topic, proto

      int more = 1;
      if (more) more = atp::zmq::receive(*socketPtr_, &frame1);

      bool continueProcess = true;
      // check the topic -- for regular data vs. book data
      if (boost::algorithm::starts_with(frame1, ENTITY_IB_MARKET_DEPTH)) {

        // MarketDepth

        if (more) atp::zmq::receive(*socketPtr_, &frame2);

        MarketDepth marketDepth;
        if (marketDepth.ParseFromString(frame2)) {

          uint64_t ts = marketDepth.timestamp();
          VARZ_marketdepth_event_interval_micros =
              ts -VARZ_marketdepth_event_last_ts;
          VARZ_marketdepth_event_last_ts = ts;

          // Convert timestamp to posix time.
          ptime t = from_time_t(ts / 1000000LL);
          time_duration micros(0, 0, 0, ts % 1000000LL);
          t += micros;

          // Compute the latency from the message's timestamp to now,
          // accounting for network transport, parsing, etc.
          ptime now = microsec_clock::universal_time();
          time_duration total_latency = now - t;

          if (offsetLatency_) {
            if (++count == 1) {
              // compute the offset
              latencyOffset = total_latency;
              MARKET_DATA_SUBSCRIBER_LOGGER << "Using latency offset "
                                            << latencyOffset;
            } else {
              // compute the true latency with the offset
              total_latency -= latencyOffset;
            }
          }

          uint64_t process_start = now_micros();
          continueProcess = process(frame1, marketDepth);

          uint64_t process_dt = now_micros() - process_start;

          VARZ_marketdepth_process_latency_micros = process_dt;
          VARZ_marketdepth_process_latency_micros_total += process_dt;
          VARZ_marketdepth_process_latency_micros_count++;
          VARZ_marketdepth_process_latency_drift_micros = now_micros() - ts;

          if (VARZ_marketdepth_process_latency_micros >=
              VARZ_marketdepth_event_interval_micros) {
            VARZ_marketdepth_process_latency_over_budget++;
          }
        } else {
          LOG(ERROR) << "Unable to parse: " << frame1 << ", " << frame2;
        }

      } else if (boost::algorithm::starts_with(frame1, "admin:")) {

        // Admin message

        if (more) more = atp::zmq::receive(*socketPtr_, &frame2);
        if (more) more = atp::zmq::receive(*socketPtr_, &frame3);

        continueProcess = handleAdminMessage(frame2, frame3);

      } else {

        // MarketData

        if (more) more = atp::zmq::receive(*socketPtr_, &frame2);

        MarketData marketData;
        if (marketData.ParseFromString(frame2)) {

          uint64_t ts = marketData.timestamp();
          VARZ_marketdata_event_interval_micros =
              ts -VARZ_marketdata_event_last_ts;
          VARZ_marketdata_event_last_ts = ts;

          // Compute the latency from the message's timestamp to now,
          // accounting for network transport, parsing, etc.

          // Convert timestamp to posix time.
          ptime t = from_time_t(ts / 1000000LL);
          time_duration micros(0, 0, 0, ts % 1000000LL);
          t += micros;

          ptime now = microsec_clock::universal_time();
          time_duration total_latency = now - t;

          if (offsetLatency_) {
            if (++count == 1) {
              // compute the offset
              latencyOffset = total_latency;
              MARKET_DATA_SUBSCRIBER_LOGGER << "Using latency offset "
                                            << latencyOffset;
            } else {
              // compute the true latency with the offset
              total_latency -= latencyOffset;
            }
          }

          uint64_t process_start = now_micros();
          continueProcess = process(frame1, marketData);

          uint64_t process_dt = now_micros() - process_start;

          VARZ_marketdata_process_latency_micros = process_dt;
          VARZ_marketdata_process_latency_micros_total += process_dt;
          VARZ_marketdata_process_latency_micros_count++;
          VARZ_marketdata_process_latency_drift_micros = now_micros() - ts;

          if (VARZ_marketdata_process_latency_micros >=
              VARZ_marketdata_event_interval_micros) {
            VARZ_marketdata_process_latency_over_budget++;
          }
        } else {
          LOG(ERROR) << "Unable to parse: " << frame1 << ", " << frame2;
        }
      }

      // Consume any additional frames
      while (more) {
        string extra;
        more = atp::zmq::receive(*socketPtr_, &extra);
        LOG(ERROR) << "Unexpected frame: " << extra;
        if (more == 0) break;
      }

      if (!continueProcess) {
        VARZ_marketdata_process_stopped = true;
        VARZ_marketdepth_process_stopped = true;
        break;
      }
    }
  }

 protected:

  /// overrides ManagedAgent
  virtual ::zmq::socket_t* getAdminSocket()
  {
    return socketPtr_;
  }

  virtual bool process(const string& topic, const MarketData& data) = 0;
  virtual bool process(const string& topic, const MarketDepth& data) = 0;


 private:
  string endpoint_;
  vector<string> subscriptions_;
  ::zmq::context_t* contextPtr_;
  ::zmq::socket_t* socketPtr_;
  bool ownContext_;
  bool offsetLatency_;
  boost::mutex mutex_;
};


} // namespace atp

#endif //ATP_MARKETDATA_SINK_H_
