
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <gtest/gtest.h>

#include <boost/asio.hpp>

#include "ib/logging_impl.hpp"
#include "ib/AsioEClientSocket.hpp"

using namespace ib::internal;

TEST(AsioEClientSocketTest, InitTest)
{
  boost::asio::io_service ioService;
  
  LOG(INFO) << "Started ioService" << std::endl;

  LoggingEWrapper *ew = new LoggingEWrapper();
  AsioEClientSocket ec(ioService, ew);

  LOG(INFO) << "Started " << ioService.run() << std::endl;
      

  EXPECT_TRUE(ec.eConnect("127.0.0.1", 4001, 0));


  while (!ec.isConnected()) {
    LOG(INFO) << "Waiting for connection setup." << std::endl;
    sleep(1);
  }

  while (true) {
    ec.reqCurrentTime();
    sleep(5);
  }
}
