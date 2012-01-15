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

using namespace std;


namespace atp {


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

      boost::uint64_t delay = now_micros() - timestamp_;

      total += atp::zmq::send_copy(*socket, topic_, true);
      total += atp::zmq::send_copy(*socket, timestamp_, true);
      total += atp::zmq::send_copy(*socket, key_, true);
      total += atp::zmq::send_copy(*socket, value_, true);
      total += atp::zmq::send_copy(*socket, delay, false);

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

class MarketDepth
{
 public:

  enum Side {
    ASK = 0,
    BID = 1
  };

  enum Operation {
    ADD    = 0,
    CHANGE = 1,
    REMOVE = 2
  };

  MarketDepth(const string& topic,
              const boost::uint64_t timestamp,
              const int side,
              const int level,
              const int operation,
              const double price,
              const int size,
              const string& mm = "L1") :
      topic_(topic + ".BOOK"),
      timestamp_(timestamp),
      side_(side == 0 ? ASK : BID),
      level_(level),
      operation_(operation == 0 ? ADD : (operation == 1) ?
                 CHANGE : REMOVE),
      price_(price),
      size_(size),
      mm_(mm)
  {
  }

  /// Dispatches the market data to the outbound socket.
  // Frame sequence:
  // 1. topic
  // 2. timestamp as a long (no ISO format)
  // 3. Side
  // 4. Level
  // 5. Operation
  // 6. Price
  // 7. Size
  // 8. MM
  // 5. latency
  inline size_t dispatch(::zmq::socket_t* socket)
  {
    size_t total = 0;
    if (socket != NULL) {

      // topic
      total += atp::zmq::send_copy(*socket, topic_, true);

      // timestamp
      total += atp::zmq::send_copy(*socket, timestamp_, true);

      // side
      switch (side_) {
        case ASK :
          total += atp::zmq::send_copy(*socket, "ASK", true);
          break;
        case BID :
          total += atp::zmq::send_copy(*socket, "BID", true);
          break;
      }

      // level
      total += atp::zmq::send_copy(*socket, level_, true);

      // operation
      switch (operation_) {
        case ADD :
          total += atp::zmq::send_copy(*socket, "ADD", true);
          break;
        case CHANGE :
          total += atp::zmq::send_copy(*socket, "CHANGE", true);
          break;
        case REMOVE :
          total += atp::zmq::send_copy(*socket, "REMOVE", true);
          break;
      }

      total += atp::zmq::send_copy(*socket, price_, true);
      total += atp::zmq::send_copy(*socket, size_, true);
      total += atp::zmq::send_copy(*socket, mm_, true);

      boost::uint64_t delay = now_micros() - timestamp_;
      total += atp::zmq::send_copy(*socket, delay, false);

      VARZ_marketdata_send_latency_micros = delay;
      VARZ_marketdata_send_latency_micros_total += delay;
      VARZ_marketdata_send_latency_micros_count++;
    }
    return total;
  }

 private:
  string topic_;
  boost::uint64_t timestamp_;
  Side side_;
  int level_;
  Operation operation_;
  double price_;
  int size_;
  string mm_;
};


} // namespace atp

#endif //ATP_MARKETDATA_SOURCE_H_
