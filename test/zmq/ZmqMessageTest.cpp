
#include <string>
#include <vector>

#include <glog/logging.h>
#include <gtest/gtest.h>

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include "ib/ib_common.hpp"
#include "zmq/ZmqMessage.hpp"
#include <ApiMessages.hpp>


using namespace atp::zmq;

class ZmqObject {
 public:
  enum zmq_type {
    publisher = ZMQ_PUB,
    subscriber = ZMQ_SUB,
    responder = ZMQ_REP,
    requester = ZMQ_REQ
  };

  ZmqObject(zmq_type type, std::string& addr)
      : context_(1),
        socket_(context_, type),
        running_(true)
  {

  }

  void stop()
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    running_ = false;
  }

 protected:
  void handleMessages()
  {
    while (running_) {

    }
  }

 private:
  zmq::context_t context_;
  zmq::socket_t socket_;
  bool running_;
  boost::mutex mutex_;
};


using namespace IBAPI;


TEST(ZmqMessageTest, MarketDataRequestTest)
{
  int64 start = now_micros();

  IBAPI::MarketDataRequest marketDataRequest(
      STK, "AAPL", SMART, USD);

  // Check that there is timestamp
  EXPECT_GE(marketDataRequest.when(), start);
  EXPECT_EQ(marketDataRequest.symbol, "AAPL");
  EXPECT_EQ(marketDataRequest.securityType, STK);
  EXPECT_EQ(marketDataRequest.exchange, SMART);
  EXPECT_EQ(marketDataRequest.currency, USD);
}
