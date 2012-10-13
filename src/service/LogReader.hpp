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

  size_t Process()
  {
    namespace p = proto::ib;
    using namespace marshall;

    LOG(INFO) << "Opening file " << logfile_;

    filtering_istream infile;
    bool isCompressed = find_first(logfile_, ".gz");
    if (isCompressed) {
      infile.push(gzip_decompressor());
    }
    infile.push(file_source(logfile_));

    if (!infile) {
      LOG(ERROR) << "Unable to open " << logfile_ << endl;
      return 0;
    }

    std::string line;
    size_t lines = 0;
    size_t matchedRecords = 0;

    timer_t last_ts = 0;
    timer_t last_log = 0;
    int last_written = 0;
    ptime last_log_t;

    // The lines are space separated, so we need to skip whitespaces.
    while (infile >> std::skipws >> line) {
      if (line.find(',') != std::string::npos) {

        LOG_READER_DEBUG << "Log entry = " << line;

        // parse the log line text into record
        log_record_t nv; // basic name /value pair
        if (!internal::line_to_record(line, nv, '=', ",")) {
          LOG(ERROR) << "unable to parse: " << line;
          continue;
        }

        // constructs any mapping of symbols and ids
        // this comes from actions the client made (eg. requestMktData)
        if (internal::is_action(nv) && internal::map_id_to_symbols(nv)) {
          continue;
        }

        if (!internal::is_supported(nv, unsupported_events_)) {
          continue;
        }

        if (!internal::check_time(nv, rth_)) {
          continue;
        }

        p::MarketData marketdata;
        if (nv >> marketdata) {

          matchedRecords++;

          ptime t = historian::as_ptime(marketdata.timestamp());
          if (last_log_t == boost::posix_time::not_a_date_time) {
            last_log_t = t;
            last_log = now_micros();
          } else {
            time_duration dt = t - last_log_t;
            timer_t now = now_micros();
            timer_t elapsed = now - last_log;
            double elapsedSec = static_cast<double>(elapsed) / 1000000.;
            if (dt.minutes() >= 15) {
              LOG(INFO) << "Currently at " << historian::to_est(t)
                        << ": in " << elapsedSec << " sec. "
                        << ": matchedRecords=" << matchedRecords;
              last_log_t = t;
              last_log = now;
            }
          }

          // Do something with the parsed marketdata
          cout << ((est_) ? historian::to_est(t) : t) << ","
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

          lines++;
          continue;
        }

        p::MarketDepth marketdepth;
        if (nv >> marketdepth) {

          matchedRecords++;

          ptime t = historian::as_ptime(marketdepth.timestamp());

          if (last_log_t == boost::posix_time::not_a_date_time) {
            last_log_t = t;
            last_log = now_micros();
          } else {
            time_duration dt = t - last_log_t;
            timer_t now = now_micros();
            timer_t elapsed = now - last_log;
            double elapsedSec = static_cast<double>(elapsed) / 1000000.;
            if (dt.minutes() >= 15) {
              LOG(INFO) << "Currently at " << historian::to_est(t)
                        << ": in " << elapsedSec << " sec. "
                        << ": matchedRecords=" << matchedRecords;
              last_log_t = t;
              last_log = now;
            }
          }

          // Do something with the marketdepth
          cout << ((est_) ? historian::to_est(t) : t) << ","
               << marketdepth.symbol() << ","
               << marketdepth.side() << ","
               << marketdepth.level() << ","
               << marketdepth.operation() << ","
               << marketdepth.price() << ","
               << marketdepth.size() << ","
               << marketdepth.mm() << endl;

          lines++;
          continue;
        }

        LOG_READER_LOGGER << "Skipping: " << line;
      }
    }
    LOG(INFO) << "Processed " << lines << " lines with "
              << matchedRecords << " records.";
    LOG_READER_LOGGER << "Finishing up -- closing file.";
    infile.clear();

    return matchedRecords;
  }


 private:
  string logfile_;
  bool rth_;
  bool est_;
  map<string, size_t> unsupported_events_;
};


} // service
} // atp

#endif //ATP_SERVICE_LOG_READER_H_
