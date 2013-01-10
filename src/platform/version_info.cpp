
#include <boost/lexical_cast.hpp>

#include <glog/logging.h>

#include "platform/version_info.hpp"
#include "zmq/ZmqUtils.hpp"

#include "constants.h"

using std::string;


namespace atp {

static atp::zmq::Version zmq_version;

const string version_info::GIT_LAST_COMMIT_HASH = _GIT_LAST_COMMIT_HASH;
const string version_info::GIT_LAST_COMMIT_MSG = _GIT_LAST_COMMIT_MSG;
const string version_info::IB_API_VERSION = _IB_API_VERSION;

void version_info::log(const string& label)
{
  string zmq_version;
  atp::zmq::version(&zmq_version);
  LOG(INFO) << "LABEL: " << label;
  LOG(INFO) << "BUILD: " << GIT_LAST_COMMIT_HASH
            << " - '"
            << GIT_LAST_COMMIT_MSG
            << "'";
  LOG(INFO) << "IB API VERSION: " << IB_API_VERSION;
  LOG(INFO) << "ZMQ VERSION: " << zmq_version;
}



} // atp
