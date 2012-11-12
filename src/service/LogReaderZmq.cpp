
#include <boost/bind.hpp>

#include <glog/logging.h>

#include "service/LogReaderZmq.hpp"
#include "zmq/ZmqUtils.hpp"


namespace atp {
namespace log_reader {


void DispatchEvents(LogReader& reader,
                    const std::string& endpoint,
                    const time_duration& duration)
{
  ::zmq::context_t ctx(1);
  ::zmq::socket_t sock(ctx, ZMQ_PUB);
  sock.bind(endpoint.c_str());
  LOG(INFO) << "Logreader bound to " << endpoint;

  visitor::MarketDataDispatcher p1(&sock);
  visitor::MarketDepthDispatcher p2(&sock);

  LogReader::marketdata_visitor_t m1 = p1;
  LogReader::marketdepth_visitor_t m2 = p2;

  size_t processed = reader.Process(m1, m2, duration);
  LOG(INFO) << "processed " << processed;

  // Finally send stop
  // Stop
  atp::zmq::send_copy(sock, DATA_END, true);
  atp::zmq::send_copy(sock, DATA_END, false);
  LOG(INFO) << "Sent stop " << DATA_END;

  sock.close();
}


boost::thread* DispatchEventsInThread(LogReader& logReader,
                                      const std::string& endpoint,
                                      const time_duration& duration)
{
  boost::thread* th = new boost::thread
      (boost::bind(&atp::log_reader::DispatchEvents,
                   logReader, endpoint, duration));
  return th;
}

} // log_reader
} // atp
