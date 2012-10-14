#ifndef ATP_SERVICE_LOG_READER_H_
#define ATP_SERVICE_LOG_READER_H_

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

#include <glog/logging.h>

#include "ib/contract_symbol.hpp"

#include "log_levels.h"
#include "proto/ib.pb.h"
#include "historian/time_utils.hpp"
#include "service/LogReaderInternal.hpp"

using namespace std;
using namespace boost::gregorian;
using namespace boost::algorithm;
using namespace boost::iostreams;
using namespace boost::posix_time;


namespace atp {
namespace log_reader {

using proto::ib::MarketData;
using proto::ib::MarketDepth;


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

/////////////////////////////////////////////////////////////
/// Simple logfile reader
class LogReader
{
 public:

  typedef boost::unordered_map<long,string> ticker_id_symbol_map_t;
  typedef boost::function< bool(const MarketData&) > marketdata_visitor_t;
  typedef boost::function< bool(const MarketDepth&) > marketdepth_visitor_t;

  LogReader(const string& logfile,
            bool rth = true,
            bool est = true,
            bool sync_stdio = false) :
      logfile_(logfile), rth_(rth), est_(est)
  {
    // TODO - this may be moved somewhere else.  Not sure if it belongs here
    std::cin.sync_with_stdio(sync_stdio);
  }

  size_t Process(marketdata_visitor_t& marketdata_visitor,
                 marketdepth_visitor_t& marketdepth_visitor);


 private:
  string logfile_;
  bool rth_;
  bool est_;
  map<string, size_t> unsupported_events_;
};


} // service
} // atp

#endif //ATP_SERVICE_LOG_READER_H_
