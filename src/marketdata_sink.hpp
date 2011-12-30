#ifndef ATP_MARKETDATA_SINK_H_
#define ATP_MARKETDATA_SINK_H_


#include <sstream>
#include <boost/cstdint.hpp>
#include <boost/date_time/gregorian/greg_month.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <boost/thread.hpp>

#include "log_levels.h"
#include "utils.hpp"
#include "varz/varz.hpp"
#include "zmq/ZmqUtils.hpp"


DEFINE_VARZ_int64(marketdata_process_latency_micros, 0, "");
DEFINE_VARZ_int64(marketdata_process_latency_micros_total, 0, "");
DEFINE_VARZ_int64(marketdata_process_latency_micros_count, 0, "");
DEFINE_VARZ_int64(marketdata_process_latency_over_budget, 0, "");
DEFINE_VARZ_int64(marketdata_process_latency_drift_micros, 0, "");

DEFINE_VARZ_bool(marketdata_process_stopped, false, "");

DEFINE_VARZ_string(marketdata_subscribes, "", "");
DEFINE_VARZ_string(marketdata_unsubscribes, "", "");

DEFINE_VARZ_int64(marketdata_event_last_ts, 0, "");
DEFINE_VARZ_int64(marketdata_event_interval_micros, 0, "");

namespace atp {

using namespace std;

class MarketDataSubscriber
{
 public:

  MarketDataSubscriber(const string& endpoint,
                       const vector<string>& subscriptions,
                       ::zmq::context_t* context = NULL) :
      endpoint_(endpoint),
      subscriptions_(subscriptions),
      contextPtr_(context),
      ownContext_(context == NULL),
      offsetLatency_(false)
  {
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
    long count = 0;
    time_duration latencyOffset;

    while (1) {

      string frame1; // topic
      string frame2; // timestamp
      string frame3; // key
      string frame4; // value
      string frame5; // latency

      int more = 1;
      if (more) more = atp::zmq::receive(*socketPtr_, &frame1);
      if (more) more = atp::zmq::receive(*socketPtr_, &frame2);
      if (more) more = atp::zmq::receive(*socketPtr_, &frame3);
      if (more) more = atp::zmq::receive(*socketPtr_, &frame4);
      if (more) more = atp::zmq::receive(*socketPtr_, &frame5);

      // Special handling of extra frames without blowing up.
      while (more) {
        string extra;
        more = atp::zmq::receive(*socketPtr_, &extra);
        LOG(ERROR) << "Unexpected frame: " << extra;
        if (more == 0) break;
      }
      boost::uint64_t ts;
      boost::uint64_t latency;

      istringstream s_ts(frame2);  s_ts >> ts;
      istringstream s_latency(frame5); s_latency >> latency;

      // Compute the interval of events
      VARZ_marketdata_event_interval_micros = ts -VARZ_marketdata_event_last_ts;
      VARZ_marketdata_event_last_ts = ts;

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
          LOG(INFO) << "Using latency offset " << latencyOffset;
        } else {
          // compute the true latency with the offset
          total_latency -= latencyOffset;
        }
      }

      boost::uint64_t process_start = now_micros();
      bool continueProcess = process(t, frame1, frame3, frame4, total_latency);
      boost::uint64_t process_dt = now_micros() - process_start;

      VARZ_marketdata_process_latency_micros = process_dt;
      VARZ_marketdata_process_latency_micros_total += process_dt;
      VARZ_marketdata_process_latency_micros_count++;
      VARZ_marketdata_process_latency_drift_micros = now_micros() - ts;

      if (VARZ_marketdata_process_latency_micros >=
          VARZ_marketdata_event_interval_micros) {
        VARZ_marketdata_process_latency_over_budget++;
      }

      if (!continueProcess) {
        VARZ_marketdata_process_stopped = true;
        break;
      }
    }
  }

 protected:

  /// Process an incoming event.
  /// utc - timestamp in UTC
  /// topic - the topic of the event e.g. AAPL.STK
  /// key - the event name e.g. ASK
  /// value - string value. Subclasses perform proper conversion based on event.
  virtual bool process(const boost::posix_time::ptime& utc, const string& topic,
                       const string& key, const string& value,
                       const boost::posix_time::time_duration& latency)
  {
    // override this to remove the warning
    LOG(WARNING) << topic << ' ' << utc << ' ' << key << '=' << value;
    return true;
  }


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
