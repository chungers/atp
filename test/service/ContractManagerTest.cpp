
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


TEST(ContractManagerTest, TestInvariants)
{
  namespace p = proto::ib;
  // basic assertions
  EXPECT_EQ(service::ContractManager::PutOption, p::Contract::PUT);
  EXPECT_EQ(service::ContractManager::CallOption, p::Contract::CALL);
}

TEST(ContractManagerTest, ContractManagerCreateAndDestroyTest)
{
  std::string p1(CM_ENDPOINT(16668));
  std::string p2(CM_EVENT_ENDPOINT(19999));

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
  namespace p = proto::ib;

  clearAssert();

  LOG(INFO) << "Starting contract manager";

  unsigned int cm_endpoint = 26668;
  unsigned int cm_publish = 29999;

  ::ContractManager cmm;
  SocketInitiator* cm = startContractManager(cmm, cm_endpoint, cm_publish);

  service::ContractManager cm_client(
      CM_ENDPOINT(cm_endpoint), CM_EVENT_ENDPOINT(cm_publish));
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
    }

    service::ContractManager::RequestId req_id;
    std::string symbol;

  } assert;

  assert.symbol = "AAPL";
  assert.req_id = 100;
  setAssert(assert);

  service::AsyncContractDetailsEnd future =
      cm_client.requestStockContractDetails(100, "AAPL");

  EXPECT_FALSE(future->is_ready());

  const p::ContractDetailsEnd& details_end = future->get(1000);

  EXPECT_FALSE(future->is_ready());

  sleep(1);
  LOG(INFO) << "Cleanup";
  delete cm;
}


TEST(ContractManagerTest, RequestStockDetails)
{
  namespace p = proto::ib;

  clearAssert();
  LOG(INFO) << "Starting contract manager";

  unsigned int cm_endpoint = 36668;
  unsigned int cm_publish = 39999;

  ::ContractManager cmm;
  SocketInitiator* cm = startContractManager(cmm, cm_endpoint, cm_publish);

  service::ContractManager cm_client(
      CM_ENDPOINT(cm_endpoint), CM_EVENT_ENDPOINT(cm_publish));
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

  service::AsyncContractDetailsEnd future =
      cm_client.requestStockContractDetails(request_id, "GOOG");
  EXPECT_FALSE(future->is_ready());

  const p::ContractDetailsEnd& details_end = future->get(2000);

  EXPECT_TRUE(future->is_ready());

  // Now try to query for the contract
  p::Contract goog;
  EXPECT_TRUE(cm_client.findContract("GOOG.STK", &goog));

  EXPECT_EQ("GOOG", goog.symbol());
  EXPECT_EQ(p::Contract::STOCK, goog.type());
  EXPECT_EQ(assert.conId, goog.id());

  sleep(1);
  LOG(INFO) << "Cleanup";
  delete cm;
}

TEST(ContractManagerTest, RequestOptionDetails)
{
  namespace p = proto::ib;
  using namespace boost::gregorian;

  clearAssert();

  LOG(INFO) << "Starting contract manager";

  unsigned int cm_endpoint = 46668;
  unsigned int cm_publish = 49999;

  ::ContractManager cmm;
  SocketInitiator* cm = startContractManager(cmm, cm_endpoint, cm_publish);

  service::ContractManager cm_client(
      CM_ENDPOINT(cm_endpoint), CM_EVENT_ENDPOINT(cm_publish));
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
      EXPECT_EQ(expiry, c.expiry);
      EXPECT_EQ(right, c.right);
      EXPECT_EQ(strike, c.strike);
      EXPECT_EQ(currency, c.currency);

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
    std::string expiry;
    std::string right;
    double strike;
    std::string currency;
  } _assert;

  service::ContractManager::RequestId request_id = 300;
  _assert.symbol = "GOOG";
  _assert.req_id = request_id;
  _assert.sec_type = "OPT";
  _assert.conId = 2234567;
  _assert.expiry = "20121220";
  _assert.right = "P";
  _assert.strike = 600.;
  _assert.currency = "USD";
  setAssert(_assert);

  service::AsyncContractDetailsEnd future =
      cm_client.requestOptionContractDetails(
          request_id, "GOOG",
          date(2012, Dec, 20),
          600.,
          service::ContractManager::PutOption);

  EXPECT_FALSE(future->is_ready());

  const p::ContractDetailsEnd& details_end = future->get(2000);

  EXPECT_TRUE(future->is_ready());

  // Now try to query for the contract
  p::Contract goog_put;
  EXPECT_TRUE(cm_client.findContract("GOOG.OPT.20121220.600.P", &goog_put));

  EXPECT_EQ("GOOG", goog_put.symbol());
  EXPECT_EQ(p::Contract::OPTION, goog_put.type());
  EXPECT_EQ(_assert.conId, goog_put.id());
  EXPECT_EQ(600., goog_put.strike().amount());
  EXPECT_EQ(2012, goog_put.expiry().year());
  EXPECT_EQ(12, goog_put.expiry().month());
  EXPECT_EQ(20, goog_put.expiry().day());
  EXPECT_EQ(service::ContractManager::PutOption, goog_put.right());

  sleep(1);
  LOG(INFO) << "Cleanup";
  delete cm;
}

