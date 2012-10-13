#ifndef ATP_SERVICE_LOG_READER_H_
#define ATP_SERVICE_LOG_READER_H_


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


/////////////////////////////////////////////////////////////
/// Simple logfile reader
class LogReader
{
 public:

  typedef boost::unordered_map<long,string> ticker_id_symbol_map_t;

  LogReader(const string& logfile,
            bool rth = true,
            bool est = true,
            bool sync_stdio = false) :
      logfile_(logfile), rth_(rth), est_(est)
  {
    std::cin.sync_with_stdio(sync_stdio);
  }

  size_t Process();


 private:
  string logfile_;
  bool rth_;
  bool est_;
  map<string, size_t> unsupported_events_;
};


} // service
} // atp

#endif //ATP_SERVICE_LOG_READER_H_
