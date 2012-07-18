
#include "zmq/Reactor.hpp"
#include "zmq/ZmqUtils.hpp"

#include <gtest/gtest.h>
#include <glog/logging.h>

#include "main/em.hpp"

#include "ib/ApiProtocolHandler.hpp"
#include "ib/api966/EClientMock.hpp"
#include "ib/api966/ostream.hpp"

#include "OrderManager.hpp"

using atp::AsyncOrderStatus;
using atp::OrderManager;

using IBAPI::SocketInitiator;

const static std::string EM_ENDPOINT("tcp://127.0.0.1:6667");
const static std::string EM_EVENT_ENDPOINT("tcp://127.0.0.1:7778");

#include "ApiProtocolHandler.cpp"

namespace ib {
namespace internal {

class OrderSubmitEClientMock : public EClientMock
{
 public:
  OrderSubmitEClientMock(EWrapper& ewrapper) : EClientMock(ewrapper)
  {
  }

  ~OrderSubmitEClientMock() {}

  virtual void placeOrder(OrderId id, const Contract &contract,
                          const Order &order)
  {
    LOG(INFO) << "OrderId = " << id << ", Contract" << contract
              << ", Order = " << order;
  }

};


ApiProtocolHandler::ApiProtocolHandler(EWrapper& ewrapper) :
    impl_(new implementation(
        &ewrapper, new OrderSubmitEClientMock(ewrapper)))
{
  LOG(INFO) << "Using mock EClient with EWrapper " << &ewrapper;
}

} // internal
} // ib


TEST(OrderManagerTest, OrderManagerCreateTest)
{
  OrderManager om(EM_ENDPOINT, EM_EVENT_ENDPOINT);
  LOG(INFO) << "OrderManager ready.";
}

TEST(OrderManagerTest, OrderManagerSendOrderTest)
{
  // SessionSettings for the initiator
  SocketInitiator::SessionSettings settings;
  // Outbound publisher endpoints for different channels
  map<int, string> outboundMap;

  EXPECT_TRUE(SocketInitiator::ParseSessionSettingsFromFlag(
      CONNECTOR_SPECS, settings));
  EXPECT_TRUE(SocketInitiator::ParseOutboundChannelMapFromFlag(
      OUTBOUND_ENDPOINTS, outboundMap));

  LOG(INFO) << "Starting initiator.";

  ExecutionManager em;
  SocketInitiator initiator(em, settings);
  bool publishToOutbound = false;

  EXPECT_TRUE(SocketInitiator::Configure(
      initiator, outboundMap, publishToOutbound));

  LOG(INFO) << "Start connections";
  initiator.start(false);

  LOG(INFO) << "Starting order manager";

  OrderManager om(EM_ENDPOINT, EM_EVENT_ENDPOINT);
  LOG(INFO) << "OrderManager ready.";
}

TEST(OrderManagerTest, OrderManagerCreateTest2)
{
  OrderManager om(EM_ENDPOINT, EM_EVENT_ENDPOINT);
  LOG(INFO) << "OrderManager ready.";
}
