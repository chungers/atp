
#include <string>
#include <vector>

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include <gtest/gtest.h>
#include <glog/logging.h>

#include "utils.hpp"
#include "historian/time_utils.hpp"
#include "service/LogReader.hpp"
#include "service/LogReaderVisitor.hpp"
#include "zmq/ZmqUtils.hpp"



using namespace std;
using namespace atp::log_reader;


static const string DATA_DIR = "test/sample_data/";
static const string LOG_FILE = "firehose.li126-61.jenkins.log.INFO.20121004.gz";
static const string PUB_ENDPOINT = "ipc://_logreader.ipc";

TEST(LogReaderTest, LoadLogFileTest)
{
  using namespace atp::log_reader;

  LogReader reader(DATA_DIR + LOG_FILE);

  atp::log_reader::visitor::MarketDataPrinter p1;
  atp::log_reader::visitor::MarketDepthPrinter p2;

  LogReader::marketdata_visitor_t m1 = p1;
  LogReader::marketdepth_visitor_t m2 = p2;

  ptime start;
  EXPECT_TRUE(historian::parse("2012-10-04 09:35:00", &start));

  LOG(INFO) << "Starting at " << start;
  size_t processed = reader.Process(m1, m2, seconds(5), start);
  LOG(INFO) << "processed " << processed;
}


void dispatch_events(::zmq::context_t* ctx, const time_duration& duration)
{
  LogReader reader(DATA_DIR + LOG_FILE);

  ::zmq::socket_t sock(*ctx, ZMQ_PUB);
  string endpoint(PUB_ENDPOINT.c_str());
  sock.connect(endpoint.c_str());

  visitor::MarketDataDispatcher p1(&sock);
  visitor::MarketDepthDispatcher p2(&sock);

  LogReader::marketdata_visitor_t m1 = p1;
  LogReader::marketdepth_visitor_t m2 = p2;

  size_t processed = reader.Process(m1, m2, duration);
  LOG(INFO) << "processed " << processed;

  sock.close();
}

TEST(LogReaderTest, ZmqVisitorTest)
{
  ::zmq::context_t* ctx = new ::zmq::context_t(1);

  ::zmq::socket_t sub(*ctx, ZMQ_SUB);
  sub.connect(PUB_ENDPOINT.c_str());

  string topic("AAPL.STK");
  sub.setsockopt(ZMQ_SUBSCRIBE, topic.c_str(), topic.length());


  boost::thread th(boost::bind(&dispatch_events, ctx, seconds(10)));


  size_t count = 0;
  while (1) {

    string frame1, frame2;
    try {
      int more = atp::zmq::receive(sub, &frame1);

      LOG(INFO) << "****************** Got frame1 " << frame1 << "," << more;

      if (more) {
        atp::zmq::receive(sub, &frame2);
      } else {
        LOG(ERROR) << "Only one frame!" << frame1;
        break;
      }
    } catch (::zmq::error_t e) {
      LOG(ERROR) << "Got exception: " << e.what();
    }

    LOG(INFO) << "Got " << frame1;
    proto::ib::MarketData proto;
    if (proto.ParseFromString(frame2)) {
      LOG(INFO) << "Market data " << proto.symbol();

      count++;
      if (count > 5) break;
    }
  }

  th.join();

  LOG(INFO) << "Deleting context";
  delete ctx;
}



