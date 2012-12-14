
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

#include "ib/internal.hpp"
#include "ib/ApiProtocolHandler.hpp"
#include "ib/api966/EClientMock.hpp"
#include "ib/api966/ostream.hpp"

#include "platform/contract_symbol.hpp"

#include "main/cm.hpp"
#include "main/em.hpp"
#include "service/ContractManager.hpp"
#include "service/OrderManager.hpp"


#include "service/ApiProtocolHandler.cpp"  // from test directory for mocks


using std::string;
using namespace boost::gregorian;

namespace service = atp::service;
namespace p = proto::ib;


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

  /// order manager
  service::OrderManager& om = getOm(em_endpoint, em_publish);
  LOG(INFO) << "OrderManager ready.";

  // Do something
  service::ContractManager::RequestId reqId(100);
  string symbol("GOOG");

  LOG(INFO) << "Sending request";

  service::AsyncContractDetailsEnd future =
      cm.requestStockContractDetails(reqId, symbol);

  LOG(INFO) << "Waiting for response";

  const p::ContractDetailsEnd& details_end = future->get(2000);


}


TEST(ClusterPrototype, MultipleEndpoints)
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

  // Problem: we must have a proxy or event publisher will
  // block on sending!  This is not ideal, because now we have
  // a single point of failure...

  LOG(INFO) << "Starting event proxy";
  ::zmq::context_t pctx(1);
  ::zmq::socket_t event_proxy_in(pctx, ZMQ_XSUB);
  event_proxy_in.bind(atp::zmq::EndPoint::tcp(1777).c_str());
  // ::zmq::socket_t event_proxy_out(pctx, ZMQ_PUB);
  // event_proxy_out.bind(atp::zmq::EndPoint::tcp(1778).c_str());
  // ::zmq::device(ZMQ_FORWARDER, &event_proxy_in, &event_proxy_out);

  unsigned int proxy_port = 1777;

  LOG(INFO) << "Event socket";
  ::zmq::socket_t event_sock(ctx, ZMQ_PUB);
  event_sock.bind(atp::zmq::EndPoint::tcp(17001).c_str());

  LOG(INFO) << "Event socket 2";
  ::zmq::socket_t event_sock2(ctx, ZMQ_PUB);
  event_sock2.bind(atp::zmq::EndPoint::tcp(16002).c_str());

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
