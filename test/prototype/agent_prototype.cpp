
#include <string>

#include <boost/bind.hpp>
#include <boost/thread.hpp>


#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include <glog/logging.h>

#include "utils.hpp"
#include "zmq/ZmqUtils.hpp"

#include "historian/time_utils.hpp"

#include "platform/marketdata_handler_proto_impl.hpp"
#include "platform/message_processor.hpp"

#include "service/LogReader.hpp"
#include "service/LogReaderVisitor.hpp"


DEFINE_int32(scan_minutes, 4, "minutes to scan after market opens");
DEFINE_int32(scan_seconds, 60, "seconds to scan after market opens");


using std::string;

using namespace boost::posix_time;
using namespace atp::log_reader;
using namespace atp::platform::callback;
using namespace atp::platform::marketdata;
using atp::platform::message_processor;
using atp::platform::types::timestamp_t;
using proto::ib::MarketData;


static const string DATA_DIR = "test/sample_data/";
static const string LOG_FILE = "firehose.li126-61.jenkins.log.INFO.20121004.gz";
static const string PUB_ENDPOINT = "ipc://_logreader.ipc";


void dispatch_events(::zmq::context_t* ctx, const time_duration& duration)
{
  LogReader reader(DATA_DIR + LOG_FILE);

  ::zmq::socket_t sock(*ctx, ZMQ_PUB);
  string endpoint(PUB_ENDPOINT.c_str());
  sock.bind(endpoint.c_str());
  LOG(INFO) << "Logreader bound to " << PUB_ENDPOINT;

  visitor::MarketDataDispatcher p1(&sock);
  visitor::MarketDepthDispatcher p2(&sock);

  LogReader::marketdata_visitor_t m1 = p1;
  LogReader::marketdepth_visitor_t m2 = p2;

  size_t processed = reader.Process(m1, m2, duration);
  LOG(INFO) << "processed " << processed;

  // Finally send stop
  // Stop
  atp::zmq::send_copy(sock, "STOP", true);
  atp::zmq::send_copy(sock, "STOP", false);
  LOG(INFO) << "Sent stop";

  sock.close();
}

template <typename V>
void aapl(const timestamp_t& ts, const V& v,
          const string& event, int* count)
{
  ptime t = historian::as_ptime(ts);
  LOG(INFO) << "Got appl " << event << " " << " = ["
            << historian::to_est(t) << ", " << v << "]";
  (*count)++;
}


bool stop_function(const string& topic, const string& message,
                   const string& label)
{
  LOG(INFO) << "Stopping with " << label;
  return false;
}


TEST(AgentPrototype, LoadDataFromLogfile)
{
  ::zmq::context_t ctx(1);

  // create the message_processor and the marketdata_handlers
  marketdata_handler<MarketData> aapl_handler;

  // states
  int bid_count = 0, ask_count = 0, last_trade_count, last_size_count = 0;
  // atp::platform::callback::double_updater d1 =
  //     boost::bind(&aapl<double>, _1, _2, "BID", &bid_count);

  // atp::platform::callback::double_updater d2 =
  //     boost::bind(&aapl<double>, _1, _2, "ASK", &ask_count);

  // atp::platform::callback::double_updater d3 =
  //     boost::bind(&aapl<double>, _1, _2, "LAST", &last_trade_count);

  // atp::platform::callback::int_updater d4 =
  //     boost::bind(&aapl<int>, _1, _2, "LAST_SIZE", &last_size_count);

  atp::platform::callback::update_event<double>::func d1 =
       boost::bind(&aapl<double>, _1, _2, "BID", &bid_count);

  atp::platform::callback::update_event<double>::func d2 =
      boost::bind(&aapl<double>, _1, _2, "ASK", &ask_count);

  atp::platform::callback::update_event<double>::func d3 =
      boost::bind(&aapl<double>, _1, _2, "LAST", &last_trade_count);

  atp::platform::callback::update_event<int>::func d4 =
      boost::bind(&aapl<int>, _1, _2, "LAST_SIZE", &last_size_count);


  aapl_handler.bind("BID", d1);
  aapl_handler.bind("ASK", d2);
  aapl_handler.bind("LAST", d3);
  aapl_handler.bind("LAST_SIZE", d4);

  // now message_processor
  message_processor::protobuf_handlers_map symbol_handlers;
  symbol_handlers.register_handler(
      "AAPL.STK", aapl_handler);
  symbol_handlers.register_handler(
      "STOP", boost::bind(&stop_function, _1, _2, "End of Log"));

  message_processor agent(PUB_ENDPOINT, symbol_handlers);

  LOG(INFO) << "Starting thread";
  boost::thread th(boost::bind(
      &dispatch_events, &ctx,
      minutes(FLAGS_scan_minutes) + seconds(FLAGS_scan_seconds)));

  sleep(5);
  th.join();
  agent.block();

  EXPECT_GT(bid_count, 10); // at least... don't have exact count
  EXPECT_GT(ask_count, 10); // at least... don't have exact count
}

