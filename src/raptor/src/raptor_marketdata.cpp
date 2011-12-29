
#include <vector>
#include <cassert>

#include "utils.hpp"
#include "marketdata.hpp"
#include "varz/varz.hpp"
#include "varz/VarzServer.hpp"
#include "zmq/ZmqUtils.hpp"
#include "raptor_marketdata.h"


using namespace std;
using namespace Rcpp;

DEFINE_VARZ_int64(marketdata_subscriber_r_callback_latency_micros, 0, "");
DEFINE_VARZ_int64(marketdata_subscriber_r_callback_last_ts, 0, "");
DEFINE_VARZ_int64(marketdata_subscriber_r_callback_interval_micros, 0, "");
DEFINE_VARZ_int64(marketdata_subscriber_r_callback_over_budget, 0, "");


namespace raptor {

class Subscriber : public atp::MarketDataSubscriber
{
 public:
  Subscriber(const string& endpoint, const vector<string>& subscriptions,
             ::zmq::context_t* context, int varzPort) :
      atp::MarketDataSubscriber(endpoint, subscriptions, context),
      varz_(varzPort, 1),
      shutdown_(false)
  {
    varz_.start();
  }

  ~Subscriber()
  {
    stop();
  }

  void start(Environment& environment, Function& callback)
  {
    environment_ = &environment;
    callback_ = &callback;
    processInbound();
  }

  void stop()
  {
    if (!shutdown_) {
      varz_.stop();
      shutdown_ = true;
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
    if (shutdown_) {
      return false;
    }
    if (callback_ != NULL && environment_ != NULL) {
      boost::uint64_t ct = (utc - utc_epoch).total_microseconds();
      double t =
          static_cast<double>(ct) / 1000000.0f;
      double delay =
          static_cast<double>(latency.total_microseconds())
        / 1000000.0f;

      try {

        boost::uint64_t start = now_micros();

        SEXP ret = (*callback_)(
            wrap(topic), wrap(t), wrap(key), wrap(value), wrap(delay));

        VARZ_marketdata_subscriber_r_callback_latency_micros =
            now_micros() - start;
        VARZ_marketdata_subscriber_r_callback_interval_micros =
            ct - VARZ_marketdata_subscriber_r_callback_last_ts;
        VARZ_marketdata_subscriber_r_callback_last_ts = ct;

        if (VARZ_marketdata_subscriber_r_callback_latency_micros >=
            VARZ_marketdata_subscriber_r_callback_interval_micros) {
          VARZ_marketdata_subscriber_r_callback_over_budget++;
        }

        return as<bool>(ret);

      } catch (std::exception e) {
        std::cerr << "Exception " << e.what() << std::endl;
        // Just stop the loop so we don't hang
        return false;
      }
    }
    return true;
  }

 private:
  Function* callback_;
  Environment* environment_;
  atp::varz::VarzServer varz_;
  bool shutdown_;
};

} // namespace raptor

/// Create a new subscriber to the given endpoint and with varz at given port.
SEXP marketdata_create_subscriber(SEXP endpoint,
                                  SEXP varzPort)
{
  zmq::context_t *context = new zmq::context_t(1);
  string ep = as<string>(endpoint);

  // Initialize with empty subscriptions
  vector<string> subscriptions;

  // Varz port
  int varz_port = as<int>(varzPort);

  raptor::Subscriber* subscriber =
      new raptor::Subscriber(ep, subscriptions, context, varz_port);

  // Construct the return handle object- we only expose the context_t and
  // the actual subscriber object.
  XPtr<zmq::context_t> contextPtr(context);
  XPtr<raptor::Subscriber> subscriberPtr(subscriber);

  // Create a new new environment for storing any state associated with
  // this subscriber.
  Environment handle = Environment::base_env().new_child(true);
  handle.assign("context", contextPtr);
  handle.assign("mdClient", subscriberPtr);
  return handle;
}

/// Starts the subscriber.  This will block.
SEXP marketdata_start_subscriber(SEXP subscriber, SEXP handler)
{
  Environment subscriberEnv(subscriber);
  XPtr<raptor::Subscriber> md_sub(subscriberEnv["mdClient"],
                                  R_NilValue, R_NilValue);
  // Callback handler
  Function callback(handler);

  assert(callback);

  // Starts the event loop.
  md_sub->start(subscriberEnv, callback);

  return wrap(R_NilValue);
}

/// Stops the subscriber.  Returns control to the main caller.
SEXP marketdata_stop_subscriber(SEXP subscriber)
{
  Environment subscriberEnv(subscriber);
  XPtr<raptor::Subscriber> md_sub(subscriberEnv["mdClient"],
                                  R_NilValue, R_NilValue);
  md_sub->stop();
  return wrap(true);
}

/// Subscribe to the market data for given list of contracts with the
/// given handler function which processes inbound market data messages.
SEXP marketdata_subscribe(SEXP subscriber, SEXP topics)
{
  Environment subscriberEnv(subscriber);
  XPtr<raptor::Subscriber> md_sub(subscriberEnv["mdClient"],
                                  R_NilValue, R_NilValue);

  // Construct the subscription topics from input contracts.
  vector<string> subscriptions;
  CharacterVector topicsVector(topics);

  int count = 0;
  for (CharacterVector::iterator topic = topicsVector.begin();
       topic != topicsVector.end();
       ++topic) {
    string topic_str(static_cast<const char*>(*topic));
    if (md_sub->subscribe(topic_str)) count++;
  }
  return wrap(count);
}

/// Unsubscribe market data for given list of contracts.
SEXP marketdata_unsubscribe(SEXP subscriber, SEXP topics)
{
  Environment subscriberEnv(subscriber);
  XPtr<raptor::Subscriber> md_sub(subscriberEnv["mdClient"],
                                  R_NilValue, R_NilValue);

  // Construct the subscription topics from input contracts.
  vector<string> subscriptions;
  CharacterVector topicsVector(topics);

  int count = 0;
  for (CharacterVector::iterator topic = topicsVector.begin();
       topic != topicsVector.end();
       ++topic) {
    string topic_str(static_cast<const char*>(*topic));
    if (md_sub->unsubscribe(topic_str)) count++;
  }
  return wrap(count);
}
