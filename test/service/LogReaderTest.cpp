
#include <string>
#include <vector>

#include <boost/assign.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include <glog/logging.h>

#include "utils.hpp"
#include "common/time_utils.hpp"
#include "service/LogReader.hpp"
#include "service/LogReaderVisitor.hpp"
#include "zmq/Subscriber.hpp"
#include "zmq/ZmqUtils.hpp"



using namespace std;
using namespace atp::log_reader;


static const string DATA_DIR = "test/sample_data/";
static const string LOG_FILE = "firehose.log.gz";


DEFINE_string(data_dir, DATA_DIR, "test data directory");
DEFINE_string(log_file, LOG_FILE, "log file name");

static const string PUB_ENDPOINT = "ipc://_logreader.ipc";

TEST(LogReaderTest, LoadLogFileTest)
{
  using namespace atp::log_reader;

  LogReader reader(FLAGS_data_dir + FLAGS_log_file);

  atp::log_reader::visitor::MarketDataPrinter p1;
  atp::log_reader::visitor::MarketDepthPrinter p2;

  LogReader::marketdata_visitor_t m1 = p1;
  LogReader::marketdepth_visitor_t m2 = p2;

  ptime start;
  ASSERT_TRUE(atp::time::parse("2012-12-31 09:35:00", &start));

  LOG(INFO) << "Starting at " << start;
  size_t processed = reader.Process(m1, m2, seconds(5), start);
  LOG(INFO) << "processed " << processed;
}


void dispatch_events(::zmq::context_t* ctx, const time_duration& duration)
{
  LogReader reader(FLAGS_data_dir + FLAGS_log_file);

  ::zmq::socket_t sock(*ctx, ZMQ_PUB);
  string endpoint(PUB_ENDPOINT.c_str());
  sock.bind(endpoint.c_str());

  visitor::MarketDataDispatcher p1(&sock);
  visitor::MarketDepthDispatcher p2(&sock);

  LogReader::marketdata_visitor_t m1 = p1;
  LogReader::marketdepth_visitor_t m2 = p2;

  size_t processed = reader.Process(m1, m2, duration);
  LOG(INFO) << "processed " << processed;

  sock.close();
}



class Subscriber : public atp::zmq::Subscriber::Strategy
{
 public:
  Subscriber(int count) : count_(count)
  {
  }

  /// check messages
  virtual bool check_message(::zmq::socket_t& socket)
  {
    string frame1, frame2;
    try {
      int more = atp::zmq::receive(socket, &frame1);

      LOG(INFO) << "****************** Got frame1 " << frame1 << "," << more;

      if (more) {
        atp::zmq::receive(socket, &frame2);
      } else {
        LOG(ERROR) << "Only one frame!" << frame1;
        return false;
      }
    } catch (::zmq::error_t e) {
      LOG(ERROR) << "Got exception: " << e.what();
    }

    LOG(INFO) << "Got " << frame1;
    proto::ib::MarketData proto;
    if (proto.ParseFromString(frame2)) {
      LOG(INFO) << "Market data " << proto.symbol() << ", " << count_;
      count_--;
    }
    return count_ > 0;
  }

 private:
  int count_;
};


TEST(LogReaderTest, ZmqVisitorTest)
{
  ::zmq::context_t ctx(1);

  LOG(INFO) << "Starting subscriber";
  vector<string> topics = boost::assign::list_of
      ("GOOG.OPT.20121005.765.C")("AAPL.STK")("GOOG.STK")("AMZN.STK");

  Subscriber strategy(100);
  atp::zmq::Subscriber sub(PUB_ENDPOINT, topics, strategy, &ctx);

  LOG(INFO) << "Starting thread";
  boost::thread th(boost::bind(&dispatch_events, &ctx, seconds(10)));

  LOG(INFO) << "Joining";
  th.join();

  LOG(INFO) << "Blocking";
  sub.block();
}



