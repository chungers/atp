#ifndef ATP_SERVICE_LOG_READER_VISITOR_H_
#define ATP_SERVICE_LOG_READER_VISITOR_H_

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include <zmq.hpp>

#include <glog/logging.h>

#include "common/time_utils.hpp"
#include "proto/ib.pb.h"
#include "historian/constants.hpp"
#include "zmq/ZmqUtils.hpp"


using namespace std;
using namespace boost::posix_time;

using proto::ib::MarketData;
using proto::ib::MarketDepth;



namespace atp {
namespace log_reader {

/////////////////////////////////////////////////////////////
/// Visitors
namespace visitor {

/// Simple visitor that prints to stdout.
class LinePrinter
{
 public:
  LinePrinter() : facet_(new time_facet("%Y-%m-%d %H:%M:%S%F%Q"))
  {
    std::cout.imbue(std::locale(std::cout.getloc(), facet_));
  }

 private:
  time_facet* facet_;
};

struct MarketDataPrinter : LinePrinter
{
  bool operator()(const proto::ib::MarketData& marketdata)
  {
    boost::posix_time::ptime t = atp::common::as_ptime(marketdata.timestamp());
    cout << atp::common::to_est(t) << ","
         << marketdata.symbol() << ","
         << marketdata.event() << ",";
    switch (marketdata.value().type()) {
      case (proto::common::Value_Type_INT) :
        cout << marketdata.value().int_value();
        break;
      case (proto::common::Value_Type_DOUBLE) :
        cout << marketdata.value().double_value();
        break;
      case (proto::common::Value_Type_STRING) :
        cout << marketdata.value().string_value();
        break;
    }
    cout << endl;
    return true;
  }
};

struct MarketDepthPrinter : LinePrinter
{
  bool operator()(const proto::ib::MarketDepth& marketdepth)
  {
    boost::posix_time::ptime t = atp::common::as_ptime(marketdepth.timestamp());
    cout << atp::common::to_est(t) << ","
         << marketdepth.symbol() << ","
         << marketdepth.side() << ","
         << marketdepth.level() << ","
         << marketdepth.operation() << ","
         << marketdepth.price() << ","
         << marketdepth.size() << ","
         << marketdepth.mm() << endl;
    return true;
  }
};

using ::zmq::context_t;
using ::zmq::socket_t;

////////////////////////////////////////////////////////////////////
/// Zmq event source
class ZmqEventSource
{
 public:
  ZmqEventSource(context_t* context,
                 const string& endpoint, bool publish = true) :
      context_(context == NULL ? new context_t(1) : context),
      endpoint_(endpoint),
      publish_(publish),
      own_context_(context == NULL),
      own_socket_(true)
  {
    socket_ = new socket_t(*context_, publish_ ? ZMQ_PUB : ZMQ_PUSH);
    if (publish_) {
      socket_->bind(endpoint_.c_str());
    } else {
      socket_->connect(endpoint_.c_str());
    }
  }

  ZmqEventSource(socket_t* shared) :
      own_context_(false),
      own_socket_(false),
      socket_(shared)
  {
  }

  const string& endpoint() const
  {
    return endpoint_;
  }

  socket_t* socket() const
  {
    return socket_;
  }

  void clean_up()
  {
    if (own_socket_ && socket_ != NULL) delete socket_;
    if (own_context_) delete context_;
  }

 protected:

  template <typename M>
  bool dispatch(const string& topic, const M& message)
  {
    string buff;
    size_t sent = 0;
    try {

      if (message.SerializeToString(&buff)) {
        sent = atp::zmq::send_copy(*socket_, topic, true);
        sent += atp::zmq::send_copy(*socket_, buff, false);
      }
      return sent > 0;

    } catch (::zmq::error_t e) {
      LOG(ERROR) << "Exception while sending " << e.what();
      return false;
    }
  }


 private:
  context_t* context_;
  string endpoint_;
  bool publish_;
  bool own_context_;
  bool own_socket_;
  socket_t* socket_;
};

struct MarketDataDispatcher : ZmqEventSource
{
  MarketDataDispatcher(context_t* context,
                       const string& endpoint, bool publish = true) :
      ZmqEventSource(context, endpoint, publish)
  {
  }

  MarketDataDispatcher(socket_t* shared) :
      ZmqEventSource(shared)
  {
  }

  bool operator()(const proto::ib::MarketData& marketdata)
  {
    const string topic = marketdata.symbol();
    return dispatch(topic, marketdata);
  }
};

struct MarketDepthDispatcher : ZmqEventSource
{
  MarketDepthDispatcher(context_t* context,
                       const string& endpoint, bool publish = true) :
      ZmqEventSource(context, endpoint, publish)
  {
  }

  MarketDepthDispatcher(socket_t* shared) :
      ZmqEventSource(shared)
  {
  }

  bool operator()(const proto::ib::MarketDepth& marketdepth)
  {
    ostringstream ss;
    ss << historian::ENTITY_IB_MARKET_DEPTH << ':' << marketdepth.symbol();
    return dispatch(ss.str(), marketdepth);
  }
};

} // visitors
} // log_reader
} // atp


#endif //ATP_SERVICE_LOG_READER_VISITOR_H_
