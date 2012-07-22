
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

const static int AAPL_CONID = 265598;
const static int GOOG_CONID = 30351181;

const static std::string EM_ENDPOINT("tcp://127.0.0.1:6667");
const static std::string EM_EVENT_ENDPOINT("tcp://127.0.0.1:8888");

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
  LOG(ERROR) << "**** Using mock EClient with EWrapper " << &ewrapper;
}

} // internal
} // ib


TEST(OrderManagerTest, OrderManagerCreateAndDestroyTest)
{
  OrderManager om(EM_ENDPOINT, EM_EVENT_ENDPOINT);
  LOG(INFO) << "OrderManager ready.";
}

// Create a EM stubb
SocketInitiator* startExecutionManager(ExecutionManager& em)
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

  SocketInitiator* initiator = new SocketInitiator(em, settings);
  bool publishToOutbound = false;

  EXPECT_TRUE(SocketInitiator::Configure(
      *initiator, outboundMap, publishToOutbound));

  LOG(INFO) << "Start connections";
  initiator->start(false);

  return initiator;
}


TEST(OrderManagerTest, OrderManagerSendOrderTest)
{
  namespace p = proto::ib;

  LOG(INFO) << "Starting order manager";

  ExecutionManager exm;
  SocketInitiator* em = startExecutionManager(exm);

  OrderManager om(EM_ENDPOINT, EM_EVENT_ENDPOINT);
  LOG(INFO) << "OrderManager ready.";

  // Create contract
  p::Contract aapl;
  aapl.set_id(AAPL_CONID);
  aapl.set_type(p::Contract::STOCK);
  aapl.set_symbol("AAPL");

  // Create base order
  p::Order baseOrder;
  baseOrder.set_id(1);
  baseOrder.set_action(p::Order::BUY);
  baseOrder.set_quantity(100);
  baseOrder.set_min_quantity(0);
  baseOrder.mutable_contract()->CopyFrom(aapl);

  // Set a market order
  p::MarketOrder marketOrder;
  marketOrder.mutable_base()->CopyFrom(baseOrder);

  AsyncOrderStatus status = om.send(marketOrder);

  sleep(2);
  LOG(INFO) << "Cleanup";
  delete em;
}

TEST(OrderManagerTest, OrderManagerCreateTest2)
{
  OrderManager om(EM_ENDPOINT, EM_EVENT_ENDPOINT);
  LOG(INFO) << "OrderManager ready.";
}
