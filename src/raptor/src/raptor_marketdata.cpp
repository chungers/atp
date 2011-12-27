
#include <vector>

#include "utils.hpp"
#include "marketdata.hpp"
#include "zmq/ZmqUtils.hpp"
#include "raptor_marketdata.h"


using namespace std;
using namespace Rcpp;

namespace raptor {

class Subscriber : public atp::MarketDataSubscriber
{
 public:
  Subscriber(const string& endpoint, const vector<string>& subscriptions,
             ::zmq::context_t* context, Function& callback) :
      atp::MarketDataSubscriber(endpoint, subscriptions, context),
      callback_(callback)
  {
    XPtr<zmq::context_t> contextPtr(context);
    XPtr<atp::MarketDataSubscriber> marketDataSubscriberPtr(this);
    handle_ = List::create(Named("context", contextPtr),
                           Named("marketdata", marketDataSubscriberPtr));
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
    double t =
        static_cast<double>((utc - utc_epoch).total_microseconds())
        / 1000000.0f;
    SEXP ret = callback_(handle_, wrap(t), wrap(topic), wrap(key), wrap(value));
    return as<bool>(ret);
  }

 private:
  Function& callback_;
  List handle_;
};

} // namespace raptor

/// Subscribe to the market data for given list of contracts with the
/// given handler function which processes inbound market data messages.
SEXP firehose_subscribe_marketdata(SEXP endpoint,
                                   SEXP topics,
                                   SEXP handler)
{
  zmq::context_t *context = new zmq::context_t(1);
  string ep = as<string>(endpoint);

  // Construct the subscription topics from input contracts.
  vector<string> subscriptions;
  CharacterVector topicsVector(topics);
  for (CharacterVector::iterator topic = topicsVector.begin();
       topic != topicsVector.end();
       ++topic) {
    string t(static_cast<const char*>(*topic));
    subscriptions.push_back(t);
  }

  Function callback(handler);
  raptor::Subscriber* subscriber =
      new raptor::Subscriber(ep, subscriptions, context, callback);

  subscriber->processInbound();
  return wrap(true);
}

/// Unsubscribe market data for given list of contracts.
SEXP firehose_unsubscribe_marketdata(SEXP source,
                                     SEXP contracts)
{
  return wrap(true);
}
