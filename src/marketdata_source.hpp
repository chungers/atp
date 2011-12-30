#ifndef ATP_MARKETDATA_SOURCE_H_
#define ATP_MARKETDATA_SOURCE_H_


#include <sstream>
#include <boost/cstdint.hpp>
#include <boost/date_time/gregorian/greg_month.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <boost/thread.hpp>

#include "log_levels.h"
#include "utils.hpp"
#include "varz/varz.hpp"
#include "zmq/ZmqUtils.hpp"


DEFINE_VARZ_int64(marketdata_send_latency_micros, 0, "");
DEFINE_VARZ_int64(marketdata_send_latency_micros_total, 0, "");
DEFINE_VARZ_int64(marketdata_send_latency_micros_count, 0, "");


namespace atp {

using namespace std;


template <typename V>
class MarketData
{
 public:
  MarketData(const string& topic,
             const boost::uint64_t timestamp,
             const string& key,
             const V& value) :
      topic_(topic), timestamp_(timestamp), key_(key), value_(value)
  {
  }

  /// Dispatches the market data to the outbound socket.
  // Frame sequence:
  // 1. topic
  // 2. timestamp as a long (no ISO format)
  // 3. key
  // 4. value
  // 5. latency
  inline size_t dispatch(::zmq::socket_t* socket)
  {
    size_t total = 0;
    if (socket != NULL) {

      std::ostringstream ts;
      ts << timestamp_;

      std::ostringstream vs;
      vs << value_;

      boost::uint64_t delay = now_micros() - timestamp_;
      std::ostringstream latency;
      latency << delay;

      total += atp::zmq::send_copy(*socket, topic_, true);
      total += atp::zmq::send_copy(*socket, ts.str(), true);
      total += atp::zmq::send_copy(*socket, key_, true);
      total += atp::zmq::send_copy(*socket, vs.str(), true);
      total += atp::zmq::send_copy(*socket, latency.str(), false);

      VARZ_marketdata_send_latency_micros = delay;
      VARZ_marketdata_send_latency_micros_total += delay;
      VARZ_marketdata_send_latency_micros_count++;
    }
    return total;
  }

 private:
  string topic_;
  boost::uint64_t timestamp_;
  string key_;
  V value_;
};


} // namespace atp

#endif //ATP_MARKETDATA_SOURCE_H_
