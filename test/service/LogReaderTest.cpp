
#include <string>
#include <vector>

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

  using namespace boost::posix_time;
  ptime epoch(boost::gregorian::date(1970,boost::gregorian::Jan,1));
  time_facet facet("%Y-%m-%d %H:%M:%S%F%Q");
  std::cout.imbue(std::locale(std::cout.getloc(), &facet));

  LogReader reader(DATA_DIR + LOG_FILE);

  size_t processed = reader.Process();

  LOG(INFO) << "processed " << processed;
}

