
#include "zmq/Reactor.hpp"
#include "zmq/ZmqUtils.hpp"

#include <gtest/gtest.h>
#include <glog/logging.h>

#include "utils.hpp"
#include "main/em.hpp"

#include "ib/ApiProtocolHandler.hpp"
#include "ib/api966/EClientMock.hpp"
#include "ib/api966/ostream.hpp"

#include "ZmqProtoBuffer.hpp"
#include "service/OrderManager.hpp"


using atp::service::AsyncOrderStatus;
using atp::service::OrderManager;

namespace p = proto::ib;

using IBAPI::SocketInitiator;

const static int AAPL_CONID = 265598;
const static int GOOG_CONID = 30351181;

static int ORDER_ID = now_micros() % 100000000;
static int GetOrderId()
{
  return ORDER_ID++;
}


std::string EM_ENDPOINT(int port = 6667)
{
  return "tcp://127.0.0.1:" + boost::lexical_cast<std::string>(port);
}
std::string EM_EVENT_ENDPOINT(int port = 8888)
{
  return "tcp://127.0.0.1:" + boost::lexical_cast<std::string>(port);
}

struct Assert
{
  virtual ~Assert() {}
  virtual void operator()(const OrderId& oid,
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

#ifdef TEST_WITH_MOCK
#include "ApiProtocolHandler.cpp"
#endif

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

///////////////////////////////////////////////////////////////////////////
#ifdef TEST_WITH_MOCK
ApiProtocolHandler::ApiProtocolHandler(EWrapper& ewrapper) :
    impl_(new implementation(
        &ewrapper, new OrderSubmitEClientMock(ewrapper)))
{
  LOG(ERROR) << "**** Using mock EClient with EWrapper " << &ewrapper;
}
#endif
///////////////////////////////////////////////////////////////////////////


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

TEST(OrderManagerTest, OrderManagerNoMockEmTest)
{
  std::string p1(EM_ENDPOINT(5667));
  std::string p2(EM_EVENT_ENDPOINT(5888));

  LOG(INFO) << p1 << ", " << p2;

  LOG(INFO) << "Starting em pull socket";
  ::zmq::context_t em_ctx(1);
  ::zmq::socket_t em(em_ctx, ZMQ_PULL);
  em.bind(p1.c_str());

  LOG(INFO) << "Starting em pub socket";
  ::zmq::context_t em_pub_ctx(1);
  ::zmq::socket_t em_pub(em_pub_ctx, ZMQ_PUB);
  em_pub.bind(p2.c_str());

  LOG(INFO) << "Starting order manager.";
  OrderManager om(p1, p2);
  LOG(INFO) << "OrderManager ready.";

  // send order
  // Create contract
  p::Contract aapl;
  aapl.set_id(AAPL_CONID);
  aapl.set_type(p::Contract::STOCK);
  aapl.set_symbol("AAPL");

  int ORDER_ID = GetOrderId();

  // Set a market order
  p::MarketOrder marketOrder;
  marketOrder.mutable_order()->set_id(100);
  marketOrder.mutable_order()->set_action(p::Order::BUY);
  marketOrder.mutable_order()->set_quantity(100);
  marketOrder.mutable_order()->set_min_quantity(0);
  marketOrder.mutable_order()->mutable_contract()->CopyFrom(aapl);

  AsyncOrderStatus future = om.send(marketOrder);

  LOG(INFO) << "Sent market order: " << future->is_ready();

  // Now check the pull socket
  string frame1, frame2;
  EXPECT_TRUE(atp::zmq::receive(em, &frame1));
  EXPECT_FALSE(atp::zmq::receive(em, &frame2));

  p::MarketOrder received;
  EXPECT_EQ(received.GetTypeName(), frame1);
  EXPECT_TRUE(received.ParseFromString(frame2));

  EXPECT_FALSE(future->is_ready());

  sleep(1);

  // Now send response
  p::OrderStatus os;
  os.set_timestamp(now_micros());
  os.set_message_id(now_micros());
  os.set_order_id(received.order().id());
  os.set_status("filled");
  os.set_filled(100);
  os.set_remaining(0);
  os.mutable_avg_fill_price()->set_amount(600.);
  os.mutable_last_fill_price()->set_amount(600.);
  os.set_client_id(100);
  os.set_perm_id(100);
  os.set_parent_id(0);
  os.set_why_held("");

  LOG(INFO) << "Publishing order status.";
  EXPECT_GE(atp::send<p::OrderStatus>(em_pub, os), 0);

  LOG(INFO) << "OrderManager checks for reply.";
  future->get(2000);
  EXPECT_TRUE(future->is_ready());
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

#ifdef TEST_WITH_MOCK
  initiator->start(false);
#else
  initiator->start();
#endif
  return initiator;
}


TEST(OrderManagerTest, OrderManagerSendOrderResponseTimeoutTest)
{
  clearAssert();

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

  int ORDER_ID = GetOrderId();

  // Set a market order
  p::MarketOrder marketOrder;
  marketOrder.mutable_order()->set_id(ORDER_ID);
  marketOrder.mutable_order()->set_action(p::Order::BUY);
  marketOrder.mutable_order()->set_quantity(100);
  marketOrder.mutable_order()->set_min_quantity(0);
  marketOrder.mutable_order()->mutable_contract()->CopyFrom(aapl);

  struct : public Assert {
    void operator()(const OrderId& orderId,
                    const Contract& contract,
                    const Order& order,
                    EWrapper& ewrapper)
    {
      EXPECT_EQ(check_order_id, orderId);
      EXPECT_EQ("AAPL", contract.symbol);
      EXPECT_EQ(AAPL_CONID, contract.conId);
      EXPECT_EQ("BUY", order.action);
      EXPECT_EQ(100, order.totalQuantity);
      EXPECT_EQ("MKT", order.orderType);
      EXPECT_EQ("IOC", order.tif);

      // Note that there's no response.  Testing for timeout
    }

    int check_order_id;
  } assert;
  assert.check_order_id = ORDER_ID;
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

TEST(OrderManagerTest, OrderManagerSendMarketOrderTest)
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

  int ORDER_ID = GetOrderId();

  // Set a market order
  p::MarketOrder marketOrder;
  marketOrder.mutable_order()->set_id(ORDER_ID);
  marketOrder.mutable_order()->set_action(p::Order::BUY);
  marketOrder.mutable_order()->set_quantity(100);
  marketOrder.mutable_order()->set_min_quantity(100);
  marketOrder.mutable_order()->mutable_contract()->CopyFrom(aapl);

  struct : public Assert {
    void operator()(const OrderId& orderId,
                    const Contract& contract,
                    const Order& order,
                    EWrapper& ewrapper)
    {
      EXPECT_EQ(check_order_id, orderId);
      EXPECT_EQ("AAPL", contract.symbol);
      EXPECT_EQ(AAPL_CONID, contract.conId);
      EXPECT_EQ("BUY", order.action);
      EXPECT_EQ(100, order.totalQuantity);
      EXPECT_EQ(100, order.minQty);
      EXPECT_EQ("MKT", order.orderType);
      EXPECT_EQ("IOC", order.tif);

      // Send back order status
      OrderId respOrderId(check_order_id);
      IBString status("filled");
      int filled = 100;
      int remaining = 0;
      double avgFillPrice = 600.;
      int permId = 0;
      int parentId = 0;
      double lastFillPrice = 600.;
      int clientId = 1;
      IBString whyHeld("");

      // Send a few crap messages but only one for the order
      // submitted.
      for (int i = 0; i < 5; ++i) {
        ewrapper.orderStatus(respOrderId + i, status, filled, remaining,
                             avgFillPrice, permId, parentId,
                             lastFillPrice, clientId, whyHeld);
      }
    }

    int check_order_id;
  } assert;

  assert.check_order_id = ORDER_ID;

  setAssert(assert);
  AsyncOrderStatus future = om.send(marketOrder);

  EXPECT_FALSE(future->is_ready());

  // This will block until received.
  const p::OrderStatus& status = future->get(5000);

  EXPECT_TRUE(future->is_ready());

  if (future->is_ready()) {
    EXPECT_EQ("filled", status.status());
    EXPECT_EQ(marketOrder.order().quantity(), status.filled());
  }

  LOG(INFO) << "Cleanup";
  delete em;
}

/// multiple responses as duplicates.
void test_limit_order_with_responses(int responses,
                                     unsigned int em_endpoint,
                                     unsigned int pub_endpoint)
{
  clearAssert();

  namespace p = proto::ib;

  LOG(INFO) << "Starting order manager";

  ExecutionManager exm;
  SocketInitiator* em = startExecutionManager(exm, em_endpoint, pub_endpoint);

  OrderManager om(EM_ENDPOINT(em_endpoint), EM_EVENT_ENDPOINT(pub_endpoint));
  LOG(INFO) << "OrderManager ready.";

  // Create contract
  p::Contract aapl;
  aapl.set_id(AAPL_CONID);
  aapl.set_type(p::Contract::STOCK);
  aapl.set_symbol("AAPL");

  int ORDER_ID = GetOrderId();

  // Set a limit order
  p::LimitOrder limitOrder;
  limitOrder.mutable_order()->set_id(ORDER_ID);
  limitOrder.mutable_order()->set_action(p::Order::BUY);
  limitOrder.mutable_order()->set_quantity(100);
  limitOrder.mutable_order()->set_min_quantity(100);
  limitOrder.mutable_order()->mutable_contract()->CopyFrom(aapl);
  limitOrder.mutable_limit_price()->set_amount(600.);

  struct : public Assert {
    void operator()(const OrderId& orderId,
                    const Contract& contract,
                    const Order& order,
                    EWrapper& ewrapper)
    {
      EXPECT_EQ(check_order_id, orderId);
      EXPECT_EQ("AAPL", contract.symbol);
      EXPECT_EQ(AAPL_CONID, contract.conId);
      EXPECT_EQ("BUY", order.action);
      EXPECT_EQ(100, order.totalQuantity);
      EXPECT_EQ(100, order.minQty);
      EXPECT_EQ("LMT", order.orderType);
      EXPECT_EQ("IOC", order.tif);
      EXPECT_EQ(600., order.lmtPrice);

      // Send back order status
      OrderId respOrderId(check_order_id);
      IBString status("filled");
      int filled = 100;
      int remaining = 0;
      double avgFillPrice = 600.;
      int permId = 0;
      int parentId = 0;
      double lastFillPrice = 600.;
      int clientId = 1;
      IBString whyHeld("");

      // Send a few crap messages but only one for the order
      // submitted.
      for (int i = 0; i < responses; ++i) {
        ewrapper.orderStatus(respOrderId + i, status, filled, remaining,
                             avgFillPrice, permId, parentId,
                             lastFillPrice, clientId, whyHeld);
      }
    }

    int check_order_id;
    int responses;
  } _assert;

  _assert.check_order_id = ORDER_ID;
  _assert.responses = responses;

  setAssert(_assert);
  AsyncOrderStatus future = om.send(limitOrder);

  EXPECT_FALSE(future->is_ready());

  // This will block until received or until timeout.
  const p::OrderStatus& status = future->get(2000);

  EXPECT_TRUE(future->is_ready());

  if (future->is_ready()) {
    EXPECT_EQ("filled", status.status());
    EXPECT_EQ(limitOrder.order().quantity(), status.filled());
  }

  LOG(INFO) << "Cleanup";
  delete em;
}


TEST(OrderManagerTest, OrderManagerSendLimitOrderSingleResponseTest)
{
  test_limit_order_with_responses(1, 6669, 8899);
}

TEST(OrderManagerTest, OrderManagerSendLimitOrderDuplicateResponseTest)
{
  test_limit_order_with_responses(10, 16669, 18899);
}

