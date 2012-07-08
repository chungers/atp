
#include "zmq/Reactor.hpp"
#include "zmq/ZmqUtils.hpp"

#include <gtest/gtest.h>
#include <glog/logging.h>


#include "OrderManager.hpp"

using atp::AsyncOrderStatus;
using atp::OrderManager;

const static std::string EM_ENDPOINT("tcp://127.0.0.1:6667");
const static std::string EM_EVENT_ENDPOINT("tcp://127.0.0.1:7778");



TEST(OrderManagerTest, OrderManagerCreateTest)
{
  OrderManager om(EM_ENDPOINT, EM_EVENT_ENDPOINT);
  LOG(INFO) << "OrderManager ready.";
}

TEST(OrderManagerTest, OrderManagerSendOrderTest)
{
  OrderManager om(EM_ENDPOINT, EM_EVENT_ENDPOINT);
  LOG(INFO) << "OrderManager ready.";
}

