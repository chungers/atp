
#include <string>
#include <iostream>
#include <vector>

#include <glog/logging.h>
#include <gtest/gtest.h>

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include "varz/varz.hpp"
#include "varz/VarzServer.hpp"

using namespace atp::varz;

TEST(VarzTest, VarzServerStartStopTest)
{
  VarzServer vs(8888, 2);
  LOG(INFO) << "Starting server";
  vs.start();
  sleep(1);
  LOG(INFO) << "Stopping server explicitly";
  vs.stop();
}

TEST(VarzTest, VarzServerStartStopTest2)
{
  VarzServer vs(8888, 2);
  LOG(INFO) << "Starting server";
  vs.start();
  sleep(1);
  LOG(INFO) << "Stopping server when server out of scope.";
}
