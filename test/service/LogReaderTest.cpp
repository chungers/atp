
#include <string>
#include <vector>

#include <boost/bind.hpp>
#include <boost/scoped_ptr.hpp>

#include <gtest/gtest.h>
#include <glog/logging.h>

#include "utils.hpp"
#include "service/LogReader.hpp"

using namespace std;

static const string DATA_DIR = "test/sample_data/";
static const string LOG_FILE = "firehose.li126-61.jenkins.log.INFO.20121004-064509.26223.gz";



TEST(LogReaderTest, LoadLogFileTest)
{
  using namespace atp::log_reader;

  // boost::scoped_ptr<time_facet> facet_(new time_facet("%Y-%m-%d %H:%M:%S%F%Q"));
  // std::cout.imbue(std::locale(std::cout.getloc(), facet_.get()));

  LogReader reader(DATA_DIR + LOG_FILE);

  atp::log_reader::visitor::MarketDataPrinter p1;
  atp::log_reader::visitor::MarketDepthPrinter p2;

  LogReader::marketdata_visitor_t m1 = p1;
  LogReader::marketdepth_visitor_t m2 = p2;

  size_t processed = reader.Process(m1, m2);

  LOG(INFO) << "processed " << processed;
}

