
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

std::string EM_ENDPOINT(int port = 6667)
{
  return "tcp://127.0.0.1:" + boost::lexical_cast<std::string>(port);
}
std::string EM_EVENT_ENDPOINT(int port = 8888)
{
  return "tcp://127.0.0.1:" + boost::lexical_cast<std::string>(port);
}

#include "ApiProtocolHandler.cpp"

struct Assert
{
  virtual ~Assert() {}
  virtual void operator()(const OrderId& o,
                          const Contract& c,
                          const Order& o,
                          EWrapper& e) = 0;
};

static Assert* ORDER_ASSERT;

void setAssert(Assert& assert)
{
  ORDER_ASSERT = &assert;
}
void clearAssert()
{
  ORDER_ASSERT = NULL;
}

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
    EXPECT_TRUE(getWrapper() != NULL);
    if (ORDER_ASSERT) {
      (*ORDER_ASSERT)(id, contract, order, *getWrapper());
    }
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
  std::string p1(EM_ENDPOINT(6667));
  std::string p2(EM_EVENT_ENDPOINT(8888));

  LOG(ERROR) << p1 << ", " << p2;
  OrderManager om(p1, p2);
  LOG(INFO) << "OrderManager ready.";
}

// Create a EM stubb
SocketInitiator* startExecutionManager(ExecutionManager& em,
                                       int reactor_port,
                                       int event_port)
{
  // SessionSettings for the initiator
  SocketInitiator::SessionSettings settings;
  // Outbound publisher endpoints for different channels
  map<int, string> outboundMap;

  EXPECT_TRUE(SocketInitiator::ParseSessionSettingsFromFlag(
      "200=127.0.0.1:4001@tcp://127.0.0.1:" +
      boost::lexical_cast<string>(reactor_port),
      settings));
  EXPECT_TRUE(SocketInitiator::ParseOutboundChannelMapFromFlag(
      "0=tcp://127.0.0.1:" +
      boost::lexical_cast<string>(event_port),
      outboundMap));

  LOG(INFO) << "Starting initiator.";

  SocketInitiator* initiator = new SocketInitiator(em, settings);
  bool publishToOutbound = true;

  EXPECT_TRUE(SocketInitiator::Configure(
      *initiator, outboundMap, publishToOutbound));

  LOG(INFO) << "Start connections";
  initiator->start(false);

  return initiator;
}


TEST(OrderManagerTest, OrderManagerSendOrderResponseTimeoutTest)
{
  clearAssert();

  namespace p = proto::ib;

  LOG(INFO) << "Starting order manager";

  ExecutionManager exm;
  SocketInitiator* em = startExecutionManager(exm, 6667, 8888);

  OrderManager om(EM_ENDPOINT(6667), EM_EVENT_ENDPOINT(8888));
  LOG(INFO) << "OrderManager ready.";

  // Create contract
  p::Contract aapl;
  aapl.set_id(AAPL_CONID);
  aapl.set_type(p::Contract::STOCK);
  aapl.set_symbol("AAPL");

  // Set a market order
  p::MarketOrder marketOrder;
  marketOrder.mutable_base()->set_id(1);
  marketOrder.mutable_base()->set_action(p::Order::BUY);
  marketOrder.mutable_base()->set_quantity(100);
  marketOrder.mutable_base()->set_min_quantity(0);
  marketOrder.mutable_base()->mutable_contract()->CopyFrom(aapl);

  struct : public Assert {
    void operator()(const OrderId& orderId,
                    const Contract& contract,
                    const Order& order,
                    EWrapper& ewrapper)
    {
      EXPECT_EQ(1, orderId);
      EXPECT_EQ("AAPL", contract.symbol);
      EXPECT_EQ(AAPL_CONID, contract.conId);
      EXPECT_EQ("BUY", order.action);
      EXPECT_EQ(100, order.totalQuantity);
      EXPECT_EQ("MKT", order.orderType);
      EXPECT_EQ("IOC", order.tif);

      // Note that there's no response.  Testing for timeout
    }
  } assert;

  setAssert(assert);
  AsyncOrderStatus future = om.send(marketOrder);

  EXPECT_FALSE(future->is_ready());

  // This will block until received.
  const p::OrderStatus& status = future->get(1000);

  EXPECT_FALSE(future->is_ready());

  sleep(2);
  LOG(INFO) << "Cleanup";
  delete em;
}

TEST(OrderManagerTest, OrderManagerSendOrderTest)
{
  clearAssert();

  namespace p = proto::ib;

  LOG(INFO) << "Starting order manager";

  ExecutionManager exm;
  SocketInitiator* em = startExecutionManager(exm, 6668, 8889);

  OrderManager om(EM_ENDPOINT(6668), EM_EVENT_ENDPOINT(8889));
  LOG(INFO) << "OrderManager ready.";

  // Create contract
  p::Contract aapl;
  aapl.set_id(AAPL_CONID);
  aapl.set_type(p::Contract::STOCK);
  aapl.set_symbol("AAPL");

  // Set a market order
  p::MarketOrder marketOrder;
  marketOrder.mutable_base()->set_id(1);
  marketOrder.mutable_base()->set_action(p::Order::BUY);
  marketOrder.mutable_base()->set_quantity(100);
  marketOrder.mutable_base()->set_min_quantity(0);
  marketOrder.mutable_base()->mutable_contract()->CopyFrom(aapl);

  struct : public Assert {
    void operator()(const OrderId& orderId,
                    const Contract& contract,
                    const Order& order,
                    EWrapper& ewrapper)
    {
      EXPECT_EQ(1, orderId);
      EXPECT_EQ("AAPL", contract.symbol);
      EXPECT_EQ(AAPL_CONID, contract.conId);
      EXPECT_EQ("BUY", order.action);
      EXPECT_EQ(100, order.totalQuantity);
      EXPECT_EQ("MKT", order.orderType);
      EXPECT_EQ("IOC", order.tif);

      // Send back order status
      OrderId respOrderId(1);
      IBString status("filled");
      int filled = 100;
      int remaining = 0;
      double avgFillPrice = 600.;
      int permId = 0;
      int parentId = 0;
      double lastFillPrice = 600.;
      int clientId = 1;
      IBString whyHeld("");

      ewrapper.orderStatus(respOrderId, status, filled, remaining,
                           avgFillPrice, permId, parentId,
                           lastFillPrice, clientId, whyHeld);
    }
  } assert;

  setAssert(assert);
  AsyncOrderStatus future = om.send(marketOrder);

  EXPECT_FALSE(future->is_ready());

  // This will block until received.
  const p::OrderStatus& status = future->get(1000);

  EXPECT_FALSE(future->is_ready());

  sleep(1);
  LOG(INFO) << "Cleanup";
  delete em;
}

