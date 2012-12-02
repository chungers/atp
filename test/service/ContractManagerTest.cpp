
#include "zmq/Reactor.hpp"
#include "zmq/ZmqUtils.hpp"

#include <gtest/gtest.h>
#include <glog/logging.h>

#include "utils.hpp"
#include "main/cm.hpp"

#include "ib/ApiProtocolHandler.hpp"
#include "ib/api966/EClientMock.hpp"
#include "ib/api966/ostream.hpp"

#include "service/ContractManager.hpp"


using namespace atp::service;
using IBAPI::SocketInitiator;

namespace p = proto::ib;
namespace service = atp::service;


std::string CM_ENDPOINT(int port = 6668)
{
  return "tcp://127.0.0.1:" + boost::lexical_cast<std::string>(port);
}
std::string CM_EVENT_ENDPOINT(int port = 9999)
{
  return "tcp://127.0.0.1:" + boost::lexical_cast<std::string>(port);
}

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

#ifdef TEST_WITH_MOCK
#include "ApiProtocolHandler.cpp"
#endif

namespace ib {
namespace internal {

class ContractDetailsEClientMock : public EClientMock
{
 public:
  ContractDetailsEClientMock(EWrapper& ewrapper) : EClientMock(ewrapper)
  {
  }

  ~ContractDetailsEClientMock() {}

  virtual void reqContractDetails(int reqId, const Contract &contract)
  {
    LOG(INFO) << "RequestId=" << reqId << ", Contract=" << contract;
    EXPECT_TRUE(getWrapper() != NULL);
    if (CONTRACT_DETAILS_ASSERT) {
      (*CONTRACT_DETAILS_ASSERT)(reqId, contract, *getWrapper());
    }
  }
};

///////////////////////////////////////////////////////////////////////////
#ifdef TEST_WITH_MOCK
ApiProtocolHandler::ApiProtocolHandler(EWrapper& ewrapper) :
    impl_(new implementation(
        &ewrapper, new ContractDetailsEClientMock(ewrapper)))
{
  LOG(ERROR) << "**** Using mock EClient with EWrapper " << &ewrapper;
}
#endif
///////////////////////////////////////////////////////////////////////////


} // internal
} // ib


TEST(ContractManagerTest, ContractManagerCreateAndDestroyTest)
{
  std::string p1(CM_ENDPOINT(6668));
  std::string p2(CM_EVENT_ENDPOINT(9999));

  LOG(ERROR) << p1 << ", " << p2;
  service::ContractManager cm(p1, p2);
  LOG(INFO) << "ContractManager ready.";
}

// Create a CM stubb
SocketInitiator* startContractManager(::ContractManager& cm,
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

  SocketInitiator* initiator = new SocketInitiator(cm, settings);
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


TEST(ContractManagerTest, ContractManagerRequestDetailsResponseTimeoutTest)
{
  clearAssert();
  namespace p = proto::ib;

  LOG(INFO) << "Starting contract manager";

  ::ContractManager cmm;
  SocketInitiator* cm = startContractManager(cmm, 6668, 9999);

  service::ContractManager cm_client(
      CM_ENDPOINT(6668), CM_EVENT_ENDPOINT(9999));
  LOG(INFO) << "ContractManager ready.";

  // create assert
  struct : public Assert {
    virtual void operator()(service::ContractManager::RequestId& reqId,
                            const ::Contract& c,
                            EWrapper& e)
    {
      EXPECT_EQ(req_id, reqId);
      EXPECT_EQ(symbol, c.symbol);
      EXPECT_EQ(0, c.conId); // Required for proper query to IB
      EXPECT_EQ(symbol, c.localSymbol);
    }

    service::ContractManager::RequestId req_id;
    std::string symbol;

  } assert;

  assert.symbol = "AAPL";
  assert.req_id = 100;
  setAssert(assert);

  service::AsyncContractDetailsEnd future = cm_client.requestContractDetails(
      100, "AAPL");

  EXPECT_FALSE(future->is_ready());

  const p::ContractDetailsEnd& details_end = future->get(1000);

  EXPECT_FALSE(future->is_ready());

  sleep(2);
  LOG(INFO) << "Cleanup";
  delete cm;
}
