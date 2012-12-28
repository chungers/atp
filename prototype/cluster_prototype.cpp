
#include <string>
#include <math.h>
#include <vector>

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <boost/thread.hpp>

#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include <glog/logging.h>

#include <zmq.hpp>

#include <Shared/EClient.h>
#include <Shared/EWrapper.h>

#include "common/time_utils.hpp"
#include "zmq/ZmqUtils.hpp"
#include "ZmqProtoBuffer.hpp"

#include "ib/internal.hpp"
#include "ib/ApiProtocolHandler.hpp"
#include "ib/api966/EClientMock.hpp"
#include "ib/api966/ostream.hpp"

#include "platform/contract_symbol.hpp"

#include "main/cm.hpp"
#include "main/em.hpp"

#include "proto/ib.pb.h"

#include "service/ContractManager.hpp"
#include "service/OrderManager.hpp"


#include "service/ApiProtocolHandler.cpp"  // from test directory for mocks


using std::string;
using namespace boost::gregorian;

namespace service = atp::service;
namespace p = proto::ib;

struct Assert
{
  virtual ~Assert() {}
  virtual void operator()(service::ContractManager::RequestId& reqId,
                          const Contract& c,
                          EWrapper& e) = 0;
};

static Assert* CONTRACT_DETAILS_ASSERT;

void setAssert(Assert& assert)
{
  CONTRACT_DETAILS_ASSERT = &assert;
}
void clearAssert()
{
  CONTRACT_DETAILS_ASSERT = NULL;
}


namespace ib {
namespace internal {

class EClientSimulator : public EClientMock
{
 public:
  EClientSimulator(EWrapper& ewrapper) : EClientMock(ewrapper)
  {
  }
  ~EClientSimulator() {}

  virtual void reqContractDetails(int reqId, const Contract &contract)
  {
    LOG(INFO) << "RequestId=" << reqId << ", Contract=" << contract;
    EXPECT_TRUE(getWrapper() != NULL);
    if (CONTRACT_DETAILS_ASSERT) {
      (*CONTRACT_DETAILS_ASSERT)(reqId, contract, *getWrapper());
    }
  }

  virtual void placeOrder(OrderId id, const Contract &contract,
                          const Order &order)
  {
    LOG(INFO) << "OrderId = " << id << ", Contract" << contract
              << ", Order = " << order;
  }
};

/// Required for linking with the simulator
ApiProtocolHandler::ApiProtocolHandler(EWrapper& ewrapper) :
    impl_(new implementation(
        &ewrapper, new EClientSimulator(ewrapper)))
{
    LOG(ERROR) << "**** Using simulated EClient with EWrapper " << &ewrapper;
}

} // internal
} // ib

// Create a CM stubb
IBAPI::SocketInitiator* startContractManager(::ContractManager& cm,
                                             int reactor_port,
                                             int event_port)
{
  // SessionSettings for the initiator
  IBAPI::SocketInitiator::SessionSettings settings;
  // Outbound publisher endpoints for different channels
  map<int, string> outboundMap;

  EXPECT_TRUE(IBAPI::SocketInitiator::ParseSessionSettingsFromFlag(
      "200=127.0.0.1:4001@tcp://127.0.0.1:" +
      boost::lexical_cast<string>(reactor_port),
      settings));
  EXPECT_TRUE(IBAPI::SocketInitiator::ParseOutboundChannelMapFromFlag(
      "0=tcp://127.0.0.1:" +
      boost::lexical_cast<string>(event_port),
      outboundMap));

  LOG(INFO) << "Starting initiator.";

  IBAPI::SocketInitiator* initiator =
      new IBAPI::SocketInitiator(cm, settings);

  bool publishToOutbound = true;

  EXPECT_TRUE(IBAPI::SocketInitiator::Configure(
      *initiator, outboundMap, publishToOutbound));

  LOG(INFO) << "Start connections";

  initiator->start(false);
  return initiator;
}

// Create a EM stubb
IBAPI::SocketInitiator* startExecutionManager(::ExecutionManager& em,
                                              int reactor_port,
                                              int event_port)
{
  // SessionSettings for the initiator
  IBAPI::SocketInitiator::SessionSettings settings;
  // Outbound publisher endpoints for different channels
  map<int, string> outboundMap;

  EXPECT_TRUE(IBAPI::SocketInitiator::ParseSessionSettingsFromFlag(
      "200=127.0.0.1:4001@tcp://127.0.0.1:" +
      boost::lexical_cast<string>(reactor_port),
      settings));
  EXPECT_TRUE(IBAPI::SocketInitiator::ParseOutboundChannelMapFromFlag(
      "0=tcp://127.0.0.1:" +
      boost::lexical_cast<string>(event_port),
      outboundMap));

  LOG(INFO) << "Starting initiator.";

  IBAPI::SocketInitiator* initiator = new IBAPI::SocketInitiator(em, settings);
  bool publishToOutbound = true;

  EXPECT_TRUE(IBAPI::SocketInitiator::Configure(
      *initiator, outboundMap, publishToOutbound));

  LOG(INFO) << "Start connections";

  initiator->start(false);
  return initiator;
}


/// Must use a singleton otherwise weird zmq threading issues will occur.
service::ContractManager& getCm(int p1, int p2)
{
  static service::ContractManager* cm =
      new service::ContractManager(
          "tcp://127.0.0.1:" + boost::lexical_cast<string>(p1),
          "tcp://127.0.0.1:" + boost::lexical_cast<string>(p2));
  return *cm;
}


/// Must use a singleton otherwise weird zmq threading issues will occur.
service::OrderManager& getOm(int p1, int p2)
{
  static service::OrderManager* om =
      new service::OrderManager(
          "tcp://127.0.0.1:" + boost::lexical_cast<string>(p1),
          "tcp://127.0.0.1:" + boost::lexical_cast<string>(p2));
  return *om;
}



TEST(ClusterPrototype, BasicSetup)
{
  atp::zmq::Version version;
  LOG(INFO) << "ZMQ " << version;

  /// Server side
  LOG(INFO) << "Starting contract manager";

  unsigned int cm_endpoint = 26668;
  unsigned int cm_publish = 29999;

  unsigned int em_endpoint = 36668;
  unsigned int em_publish = 39999;

  ::ContractManager cmm;
  IBAPI::SocketInitiator* cm_server =
      startContractManager(cmm, cm_endpoint, cm_publish);

  ::ExecutionManager emm;
  IBAPI::SocketInitiator* em_server =
      startExecutionManager(emm, em_endpoint, em_publish);

  /// Client side
  service::ContractManager& cm = getCm(cm_endpoint, cm_publish);
  LOG(INFO) << "ContractManager ready.";

  // create assert
  struct : public Assert {
    virtual void operator()(service::ContractManager::RequestId& reqId,
                            const ::Contract& c,
                            EWrapper& ewrapper)
    {
      EXPECT_EQ(req_id, reqId);
      EXPECT_EQ(symbol, c.symbol);
      EXPECT_EQ(0, c.conId); // Required for proper query to IB
      EXPECT_EQ(symbol, c.localSymbol);
      EXPECT_EQ(sec_type, c.secType);

      sleep(1); // to avoid race for the test.  simulate some delay

      ::ContractDetails details;
      details.summary = c;
      details.summary.conId = conId;
      ewrapper.contractDetails(reqId, details);

      // send the end
      ewrapper.contractDetailsEnd(reqId);
    }

    service::ContractManager::RequestId req_id;
    std::string symbol;
    std::string sec_type;
    int conId;
  } assert;

  service::ContractManager::RequestId request_id = 200;
  assert.symbol = "GOOG";
  assert.req_id = request_id;
  assert.sec_type = "STK";
  assert.conId = 12345;

  setAssert(assert);

  /// order manager
  service::OrderManager& om = getOm(em_endpoint, em_publish);
  LOG(INFO) << "OrderManager ready.";

  // Do something
  string symbol("GOOG");

  LOG(INFO) << "Sending request";

  service::AsyncContractDetailsEnd future =
      cm.requestStockContractDetails(request_id, symbol);

  LOG(INFO) << "Waiting for response";

  const p::ContractDetailsEnd& details_end = future->get(2000);

  EXPECT_TRUE(future->is_ready());
}


TEST(ClusterPrototype, SubscribeToMultipleEndpoints)
{
  /// Server side
  LOG(INFO) << "Starting contract manager";

  unsigned int cm_endpoint = 46668;
  unsigned int cm_publish = 49999;

  unsigned int em_endpoint = 56668;
  unsigned int em_publish = 59999;

  ::ContractManager cmm;
  IBAPI::SocketInitiator* cm_server =
      startContractManager(cmm, cm_endpoint, cm_publish);

  ::ExecutionManager emm;
  IBAPI::SocketInitiator* em_server =
      startExecutionManager(emm, em_endpoint, em_publish);

  // Try a generic subscriber to both
  LOG(INFO) << "Subscriber to multiple endpoints";
  ::zmq::context_t ctx(1);
  ::zmq::socket_t sock(ctx, ZMQ_SUB);

  sock.connect(atp::zmq::EndPoint::tcp(cm_publish).c_str());
  sock.connect(atp::zmq::EndPoint::tcp(em_publish).c_str());

  // Nice: won't hang if no publisher running at that endpoint.
  for (int i = 0; i < 100; ++i) {
    sock.connect(atp::zmq::EndPoint::tcp(17000 + i).c_str());
  }
  // Subscription doesn't hang either.
  string event("event");
  sock.setsockopt(ZMQ_SUBSCRIBE, event.c_str(), event.length());

  sleep(3);

  // Start up even producers later...

  LOG(INFO) << "Event socket";
  ::zmq::socket_t event_sock(ctx, ZMQ_PUB);
  event_sock.bind(atp::zmq::EndPoint::tcp(17000 + 1).c_str());

  LOG(INFO) << "Event socket 2";
  ::zmq::socket_t event_sock2(ctx, ZMQ_PUB);
  event_sock2.bind(atp::zmq::EndPoint::tcp(17000 + 2).c_str());

  // Note that event2 is on a port that's not connected by any subscriber
  // But the sending of the messages shouldn't block.
  LOG(INFO) << "Sending events.";
  for (int i = 0; i < 1000; ++i) {
    atp::zmq::send_copy(event_sock,
                        "event" + boost::lexical_cast<string>(i), false);
    atp::zmq::send_copy(event_sock2,
                        "event" + boost::lexical_cast<string>(i), false);
  }

  for (int i = 0; i < 1000; ++i) {
    atp::zmq::send_copy(event_sock2,
                        "event" + boost::lexical_cast<string>(i), false);
  }

  // Note: not sure if it's good idea to use a proxy -- since if the proxy
  // is down, sending events may block.  Need to prototype with 3.2 and see
  // if the behavior is different.  Right now, given the current version,
  // the best may be to have many publishers of events and have a event tracker
  // that subscribes to a range of port numbers.
}


void startSimpleSubscriber(unsigned int port, const string& id)
{
  ::zmq::context_t ctx(1);
  ::zmq::socket_t sock(ctx, ZMQ_SUB);
  try {
    sock.connect(atp::zmq::EndPoint::tcp(port).c_str());
    sock.connect(atp::zmq::EndPoint::tcp(port+1).c_str());
    string event("event");
    sock.setsockopt(ZMQ_SUBSCRIBE, event.c_str(), event.length());
    string stop("stop");
    sock.setsockopt(ZMQ_SUBSCRIBE, stop.c_str(), stop.length());

    while (true) {
      string buff;
      atp::zmq::receive(sock, &buff);
      LOG(INFO) << id << ": Got " << buff;

      if (buff == "stop") {
        LOG(INFO) << id << ": Stopping because told so.";
        break;
      }
    }

  } catch (::zmq::error_t e) {
    LOG(ERROR) << id << ": Exception " << e.what();
  }
}

TEST(ClusterPrototype, PubSubSingleFrameTest)
{
  unsigned int port = 47779;


  // Note that with proxy, the publishers CONNECT to a known port.
  // Even if the proxy isn't running, the connect will not block.
  ::zmq::context_t ctx(1);
  LOG(INFO) << "Event socket";
  ::zmq::socket_t event_sock(ctx, ZMQ_PUB);
  event_sock.bind(atp::zmq::EndPoint::tcp(port).c_str());

  LOG(INFO) << "Event socket 2";
  ::zmq::socket_t event_sock2(ctx, ZMQ_PUB);
  event_sock2.bind(atp::zmq::EndPoint::tcp(port+1).c_str());

  // At the point, still no proxy. Generating events anyway; shouldn't block.
  LOG(INFO) << "Sending events without subscribers running.";

  int block = 10;
  int total = 30;
  for (int i = 0; i < block; ++i) {
    atp::zmq::send_copy(event_sock,
                        "event-sf-" +
                        boost::lexical_cast<string>(i), false);
    atp::zmq::send_copy(event_sock2,
                        "event-sf-" + boost::lexical_cast<string>(i), false);
    LOG(INFO) << "Published event " << i;
  }

  sleep(3);
  LOG(INFO) << "Starting subscribers.";
  boost::thread subscriber(boost::bind(&startSimpleSubscriber, port,
                                       "simple1"));
  boost::thread subscriber2(boost::bind(&startSimpleSubscriber, port,
                                       "simple2"));

  sleep(2);

  LOG(INFO) << "Now sending more events, with subscriber running.";

  for (int i = block; i < total; ++i) {
    atp::zmq::send_copy(event_sock,
                        "event-sf-" + boost::lexical_cast<string>(i), false);
    atp::zmq::send_copy(event_sock2,
                        "event-sf-" + boost::lexical_cast<string>(i), false);
    LOG(INFO) << "Published event " << i;
  }

  atp::zmq::send_copy(event_sock, "stop", false);

  subscriber.join();
  subscriber2.join();
}

void startMarketDataSubscriber(unsigned int port, const string& id)
{
  ::zmq::context_t ctx(1);
  ::zmq::socket_t sock(ctx, ZMQ_SUB);
  try {
    sock.connect(atp::zmq::EndPoint::tcp(port).c_str());
    sock.connect(atp::zmq::EndPoint::tcp(port+1).c_str());

    LOG(INFO) << id << ": connected to " << port;

    proto::ib::MarketData md;
    string event(md.GetTypeName());
    sock.setsockopt(ZMQ_SUBSCRIBE, event.c_str(),event.length());

    LOG(INFO) << id << ": subscribe to " << event;

    string stop("stop");
    sock.setsockopt(ZMQ_SUBSCRIBE, stop.c_str(), stop.length());

    LOG(INFO) << id << ": subscribe to " << stop;

    while (true) {
      string buff;

      LOG(INFO) << id << ":about to recv";
      atp::zmq::receive(sock, &buff);
      LOG(INFO) << "Got " << buff;
      if (buff == event) {

        string buff2;
        atp::zmq::receive(sock, &buff2);

        proto::ib::MarketData m;
        if (m.ParseFromString(buff2)) {
          LOG(INFO) << id << ": Received marketdata: " << m.symbol()
                    << ',' << m.event();
        } else {
          LOG(INFO) << id << ": Cannot parse " << buff2;
        }

      } else if (buff == "stop") {
        LOG(INFO) << id << ": Stopping because told so.";
        break;
      }
    }

  } catch (::zmq::error_t e) {
    LOG(ERROR) << "Exception " << e.what();
  }
}


TEST(ClusterPrototype, PubSubMultiFrameTest)
{
  unsigned int port = 37779;

  // Note that with proxy, the publishers CONNECT to a known port.
  // Even if the proxy isn't running, the connect will not block.
  ::zmq::context_t ctx(1);
  LOG(INFO) << "Event socket";
  ::zmq::socket_t event_sock(ctx, ZMQ_PUB);
  event_sock.bind(atp::zmq::EndPoint::tcp(port).c_str());

  LOG(INFO) << "Event socket 2";
  ::zmq::socket_t event_sock2(ctx, ZMQ_PUB);
  event_sock2.bind(atp::zmq::EndPoint::tcp(port+1).c_str());

  // At the point, still no proxy. Generating events anyway; shouldn't block.
  LOG(INFO) << "Sending events without subscribers running.";

  int block = 10;
  int total = 30;
  for (int i = 0; i < block; ++i) {

    string f1, f2;
    proto::ib::MarketData bid;
    bid.set_timestamp(now_micros());
    bid.set_symbol("AAPL.STK");
    bid.set_event("BID");
    bid.mutable_value()->set_type(proto::common::Value::DOUBLE);
    bid.mutable_value()->set_double_value(600. + i);
    bid.set_contract_id(12345);

    f1 = bid.GetTypeName();
    EXPECT_TRUE(bid.SerializeToString(&f2));
    atp::zmq::send_copy(event_sock, f1, true);
    atp::zmq::send_copy(event_sock, f2, false);

    bid.set_symbol("GOOG.STK");
    f1 = bid.GetTypeName();
    EXPECT_TRUE(bid.SerializeToString(&f2));
    atp::zmq::send_copy(event_sock2, f1, true);
    atp::zmq::send_copy(event_sock2, f2, false);

    LOG(INFO) << "Published event " << i;
  }

  sleep(3);
  LOG(INFO) << "Starting subscribers.";
  boost::thread subscriber(boost::bind(&startMarketDataSubscriber, port,
                                       "sub1"));
  boost::thread subscriber2(boost::bind(&startMarketDataSubscriber, port,
                                       "sub2"));

  sleep(2);

  LOG(INFO) << "Now sending more events, with subscriber running.";

  for (int i = block; i < total; ++i) {
    string f1, f2;
    proto::ib::MarketData ask;
    ask.set_timestamp(now_micros());
    ask.set_symbol("AAPL.STK");
    ask.set_event("ASK");
    ask.mutable_value()->set_type(proto::common::Value::DOUBLE);
    ask.mutable_value()->set_double_value(600. + i);
    ask.set_contract_id(12345);

    f1 = ask.GetTypeName();
    EXPECT_TRUE(ask.SerializeToString(&f2));
    atp::zmq::send_copy(event_sock, f1, true);
    atp::zmq::send_copy(event_sock, f2, false);

    ask.set_symbol("GOOG.STK");
    f1 = ask.GetTypeName();
    EXPECT_TRUE(ask.SerializeToString(&f2));
    atp::zmq::send_copy(event_sock2, f1, true);
    atp::zmq::send_copy(event_sock2, f2, false);

    LOG(INFO) << "Published event " << i;
  }

  atp::zmq::send_copy(event_sock, "stop", false);

  subscriber.join();
  subscriber2.join();

  // Apprently the proxy will not store messages when there are no subscribers.
  // The subscriber joins late and receives only the last batch of 10*2 messages
  // from the producer sockets.
}


#ifdef ZMQ_3X

void startProxy(::zmq::context_t* pctx,
                unsigned int proxy_xsub, unsigned int proxy_xpub)
{
  // Note that for pub/sub proxy, the proxy binds to the inbound
  // and outbound ports while the publishers and subscribers all
  // connect to the ports.
  ::zmq::socket_t event_proxy_in(*pctx, ZMQ_XSUB);
  ::zmq::socket_t event_proxy_out(*pctx, ZMQ_XPUB);

  LOG(INFO) << "Bind at both ends";
  event_proxy_in.bind(atp::zmq::EndPoint::tcp(proxy_xsub).c_str());
  event_proxy_out.bind(atp::zmq::EndPoint::tcp(proxy_xpub).c_str());

  LOG(INFO) << "connect frontend and backend -- start to block";

  // Blocks
  zmq_proxy((void*)event_proxy_in, (void*)event_proxy_out, NULL);
}

void startSubscriber(unsigned int proxy_xpub)
{
  ::zmq::context_t ctx(1);
  ::zmq::socket_t sock(ctx, ZMQ_SUB);
  try {
    sock.connect(atp::zmq::EndPoint::tcp(proxy_xpub).c_str());
    string event("event");
    sock.setsockopt(ZMQ_SUBSCRIBE, event.c_str(), event.length());
    string stop("stop");
    sock.setsockopt(ZMQ_SUBSCRIBE, stop.c_str(), stop.length());

    while (true) {
      string buff;
      atp::zmq::receive(sock, &buff);
      LOG(INFO) << "Got " << buff;

      if (buff == "stop") {
        LOG(INFO) << "Stopping because told so.";
        break;
      }
    }

  } catch (::zmq::error_t e) {
    LOG(ERROR) << "Exception " << e.what();
  }
}

TEST(ClusterPrototype, EventProxy)
{
  unsigned int proxy_xsub = 17778;
  unsigned int proxy_xpub = 17779;


  // Note that with proxy, the publishers CONNECT to a known port.
  // Even if the proxy isn't running, the connect will not block.
  ::zmq::context_t ctx(1);
  LOG(INFO) << "Event socket";
  ::zmq::socket_t event_sock(ctx, ZMQ_PUB);
  event_sock.connect(atp::zmq::EndPoint::tcp(proxy_xsub).c_str());

  LOG(INFO) << "Event socket 2";
  ::zmq::socket_t event_sock2(ctx, ZMQ_PUB);
  event_sock2.connect(atp::zmq::EndPoint::tcp(proxy_xsub).c_str());

  // At the point, still no proxy. Generating events anyway; shouldn't block.
  LOG(INFO) << "Sending events without proxy or subscriber running.";

  int block = 10;
  int total = 30;
  for (int i = 0; i < block; ++i) {
    atp::zmq::send_copy(event_sock,
                        "event-sock-" + boost::lexical_cast<string>(i), false);
    atp::zmq::send_copy(event_sock2,
                        "event-sock2-" + boost::lexical_cast<string>(i), false);
  }

  sleep(3);

  // Start a proxy after some events have been sent.
  LOG(INFO) << "Starting event proxy";
  ::zmq::context_t pctx(1);
  boost::thread proxy(boost::bind(&startProxy, &pctx, proxy_xsub, proxy_xpub));

  sleep(1);

  LOG(INFO) << "Now sending events with a running proxy.  Still no subscriber.";

  for (int i = block; i < block*2; ++i) {
    atp::zmq::send_copy(event_sock2,
                        "event-sock2-" + boost::lexical_cast<string>(i), false);
    atp::zmq::send_copy(event_sock,
                        "event-sock-" + boost::lexical_cast<string>(i), false);
  }

  sleep(2);

  LOG(INFO) << "Starting subscriber.";
  boost::thread subscriber(boost::bind(&startSubscriber, proxy_xpub));

  sleep(2);

  LOG(INFO) << "Now sending more events, with proxy and subscriber running.";

  for (int i = block*2; i < total; ++i) {
    atp::zmq::send_copy(event_sock2,
                        "event-sock2-" + boost::lexical_cast<string>(i), false);
    atp::zmq::send_copy(event_sock,
                        "event-sock-" + boost::lexical_cast<string>(i), false);
  }

  atp::zmq::send_copy(event_sock, "stop", false);

  subscriber.join();

  // Apprently the proxy will not store messages when there are no subscribers.
  // The subscriber joins late and receives only the last batch of 10*2 messages
  // from the producer sockets.
}

#endif
