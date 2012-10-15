#ifndef ATP_SERVICE_LOG_READER_VISITOR_H_
#define ATP_SERVICE_LOG_READER_VISITOR_H_

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include "proto/ib.pb.h"
#include "historian/time_utils.hpp"

using namespace std;
using namespace boost::posix_time;

using proto::ib::MarketData;
using proto::ib::MarketDepth;



namespace atp {
namespace log_reader {

/////////////////////////////////////////////////////////////
/// Visitors
namespace visitor {






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

} // visitors
} // log_reader
} // atp


#endif //ATP_SERVICE_LOG_READER_VISITOR_H_
