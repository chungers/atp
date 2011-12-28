
#include <vector>
#include <cassert>

#include "utils.hpp"
#include "marketdata.hpp"
#include "varz/VarzServer.hpp"
#include "zmq/ZmqUtils.hpp"
#include "raptor_marketdata.h"


using namespace std;
using namespace Rcpp;

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

  void start(Function& callback)
  {
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
    if (callback_ != NULL) {
      double t =
          static_cast<double>((utc - utc_epoch).total_microseconds())
          / 1000000.0f;
      double delay =
          static_cast<double>(latency.total_microseconds())
        / 1000000.0f;

      SEXP ret = (*callback_)(
          wrap(topic), wrap(t), wrap(key), wrap(value), wrap(delay));
      return as<bool>(ret);
    }
    return true;
  }

 private:
  Function* callback_;
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
  List handle = List::create(Named("context", contextPtr),
                             Named("mdClient", subscriberPtr));
  return handle;
}

/// Starts the subscriber.  This will block.
SEXP marketdata_start_subscriber(SEXP subscriber, SEXP handler)
{
  List subscriberList(subscriber);
  XPtr<raptor::Subscriber> md_sub(subscriberList["mdClient"],
                                  R_NilValue, R_NilValue);
  // Callback handler
  Function callback(handler);

  assert(callback);

  // Starts the event loop.
  md_sub->start(callback);

  return wrap(R_NilValue);
}

/// Stops the subscriber.  Returns control to the main caller.
SEXP marketdata_stop_subscriber(SEXP subscriber)
{
  List subscriberList(subscriber);
  XPtr<raptor::Subscriber> md_sub(subscriberList["mdClient"],
                                  R_NilValue, R_NilValue);
  md_sub->stop();
  return wrap(true);
}

/// Subscribe to the market data for given list of contracts with the
/// given handler function which processes inbound market data messages.
SEXP marketdata_subscribe(SEXP subscriber, SEXP topics)
{
  List subscriberList(subscriber);
  XPtr<raptor::Subscriber> md_sub(subscriberList["mdClient"],
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
  List subscriberList(subscriber);
  XPtr<raptor::Subscriber> md_sub(subscriberList["mdClient"],
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
