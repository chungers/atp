
#include <string>
#include <vector>


#include <gtest/gtest.h>
#include <glog/logging.h>

#include "utils.hpp"
#include "service/LogReader.hpp"
#include "service/LogReaderVisitor.hpp"
#include "zmq/ZmqUtils.hpp"



using namespace std;

static const string DATA_DIR = "test/sample_data/";
static const string LOG_FILE = "firehose.li126-61.jenkins.log.INFO.20121004.gz";



TEST(LogReaderTest, LoadLogFileTest)
{
  using namespace atp::log_reader;

  LogReader reader(DATA_DIR + LOG_FILE);

  atp::log_reader::visitor::MarketDataPrinter p1;
  atp::log_reader::visitor::MarketDepthPrinter p2;

  LogReader::marketdata_visitor_t m1 = p1;
  LogReader::marketdepth_visitor_t m2 = p2;

  // size_t processed = reader.Process(m1, m2);
  // LOG(INFO) << "processed " << processed;

}

TEST(LogReaderTest, ZmqVisitorTest)
{
  using namespace atp::log_reader;

  LogReader reader(DATA_DIR + LOG_FILE);

  ::zmq::context_t ctx(1);
  ::zmq::socket_t sock(ctx, ZMQ_PUB);
  string endpoint("ipc://_logreader.ipc");
  sock.connect(endpoint.c_str());

  visitor::MarketDataDispatcher p1(&sock);
  visitor::MarketDepthDispatcher p2(&sock);

  LogReader::marketdata_visitor_t m1 = p1;
  LogReader::marketdepth_visitor_t m2 = p2;

  size_t processed = reader.Process(m1, m2, minutes(2));

  LOG(INFO) << "processed " << processed;

}



