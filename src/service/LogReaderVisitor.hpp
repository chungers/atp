#ifndef ATP_SERVICE_LOG_READER_VISITOR_H_
#define ATP_SERVICE_LOG_READER_VISITOR_H_

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include <zmq.hpp>

#include "proto/ib.pb.h"
#include "historian/time_utils.hpp"
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
    std::cout.imbue(std::locale(std::cout.getloc(), facet_.get()));
  }

 private:
  boost::shared_ptr<time_facet> facet_;
};

struct MarketDataPrinter : LinePrinter
{
  bool operator()(const proto::ib::MarketData& marketdata)
  {
    boost::posix_time::ptime t = historian::as_ptime(marketdata.timestamp());
    cout << historian::to_est(t) << ","
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
    boost::posix_time::ptime t = historian::as_ptime(marketdepth.timestamp());
    cout << historian::to_est(t) << ","
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

class ZmqEventSource
{
 public:
  ZmqEventSource(context_t* context,
                 const string& endpoint, bool publish = true) :
      context_(context == NULL ? new context_t(1) : context),
      endpoint_(endpoint),
      publish_(publish),
      own_context_(context == NULL)
  {
    socket_ = new socket_t(*context_, publish_ ? ZMQ_PUB : ZMQ_PUSH);
    socket_->connect(endpoint_.c_str());
  }

  const string& endpoint() const
  {
    return endpoint_;
  }


  ~ZmqEventSource()
  {
    if (socket_ != NULL) delete socket_;
    if (own_context_) delete context_;
  }



 private:
  context_t* context_;
  string endpoint_;
  bool publish_;
  bool own_context_;
  socket_t* socket_;
};


} // visitors
} // log_reader
} // atp


#endif //ATP_SERVICE_LOG_READER_VISITOR_H_
