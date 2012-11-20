#ifndef ATP_SERVICE_LOG_READER_H_
#define ATP_SERVICE_LOG_READER_H_

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

#include <glog/logging.h>

#include "log_levels.h"
#include "proto/ib.pb.h"
#include "common/time_utils.hpp"


using namespace std;
using namespace boost::algorithm;
using namespace boost::posix_time;


namespace atp {
namespace log_reader {

typedef boost::posix_time::ptime time_t;
typedef boost::posix_time::time_duration time_duration_t;
typedef boost::posix_time::time_period time_period_t;

using proto::ib::MarketData;
using proto::ib::MarketDepth;


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

  /// Process the logfile with the given visitors for marketdata and marketdepth
  size_t Process(marketdata_visitor_t& marketdata_visitor,
                 marketdepth_visitor_t& marketdepth_visitor,
                 const time_duration_t& duration = pos_infin,
                 const time_t& start = neg_infin);


 private:
  string logfile_;
  bool rth_;
  bool est_;
  map<string, size_t> unsupported_events_;
};



} // service
} // atp

#endif //ATP_SERVICE_LOG_READER_H_
