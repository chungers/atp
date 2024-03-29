
#include <vector>
#include <cassert>

#include "utils.hpp"
#include "marketdata_sink.hpp"

#include "historian/historian.hpp"
#include "varz/varz.hpp"
#include "zmq/ZmqUtils.hpp"
#include "mds.h"


using namespace std;
using namespace Rcpp;

DEFINE_VARZ_int64(mds_subscriber_r_callback_latency_micros, 0, "");
DEFINE_VARZ_int64(mds_subscriber_r_callback_last_ts, 0, "");
DEFINE_VARZ_int64(mds_subscriber_r_callback_interval_micros, 0, "");
DEFINE_VARZ_int64(mds_subscriber_r_callback_over_budget, 0, "");


namespace raptor {

using proto::common::Value;
using proto::ib::MarketData;
using proto::ib::MarketDepth;

class Subscriber : public atp::MarketDataSubscriber
{
 public:
  Subscriber(const string& id,
             const string& adminEndpoint, const string& eventEndpoint,
             const string& endpoint, const vector<string>& subscriptions,
             int varzPort,
             ::zmq::context_t* context) :
      atp::MarketDataSubscriber(id, adminEndpoint, eventEndpoint,
                                endpoint, subscriptions, varzPort, context)
  {
    registerHandler("stop", boost::bind(&Subscriber::stop, this));
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

 protected:

  virtual bool process(const string& topic, const MarketDepth& data)
  {
    return true;
  }

  virtual bool process(const string& topic, const MarketData& data)
  {
    if (callback_ != NULL && environment_ != NULL) {
      boost::uint64_t ct = data.timestamp();
      double t =
          static_cast<double>(ct) / 1000000.0f;

      try {

        boost::uint64_t start = now_micros();

        SEXP ret = (*callback_)(
            wrap(data.symbol()), wrap(t),
            wrap(data.event()), wrap(data.value().double_value()));

        VARZ_mds_subscriber_r_callback_latency_micros =
            now_micros() - start;
        VARZ_mds_subscriber_r_callback_interval_micros =
            ct - VARZ_mds_subscriber_r_callback_last_ts;
        VARZ_mds_subscriber_r_callback_last_ts = ct;

        if (VARZ_mds_subscriber_r_callback_latency_micros >=
            VARZ_mds_subscriber_r_callback_interval_micros) {
          VARZ_mds_subscriber_r_callback_over_budget++;
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
};

} // namespace raptor

/// Create a new subscriber to the given endpoint and with varz at given port.
SEXP mds_create_subscriber(SEXP id,
                           SEXP adminEndpoint,
                           SEXP eventEndpoint,
                           SEXP endpoint,
                           SEXP varzPort)
{
  zmq::context_t *context = new zmq::context_t(1);
  string m_id = as<string>(id);
  string m_adminEndpoint = as<string>(adminEndpoint);
  string m_eventEndpoint = as<string>(eventEndpoint);
  string ep = as<string>(endpoint);

  // Initialize with empty subscriptions
  vector<string> subscriptions;

  // Varz port
  int varz_port = as<int>(varzPort);

  raptor::Subscriber* subscriber =
      new raptor::Subscriber(m_id, m_adminEndpoint, m_eventEndpoint,
                             ep, subscriptions, varz_port, context);

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
SEXP mds_start_subscriber(SEXP subscriber, SEXP handler)
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
SEXP mds_stop_subscriber(SEXP subscriber)
{
  Environment subscriberEnv(subscriber);
  XPtr<raptor::Subscriber> md_sub(subscriberEnv["mdClient"],
                                  R_NilValue, R_NilValue);
  md_sub->stop();
  return wrap(true);
}

/// Subscribe to the market data for given list of contracts with the
/// given handler function which processes inbound market data messages.
SEXP mds_subscribe(SEXP subscriber, SEXP topics)
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
SEXP mds_unsubscribe(SEXP subscriber, SEXP topics)
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
