
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

#include <Shared/EClient.h>
#include <Shared/EWrapper.h>

#include "common/time_utils.hpp"

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
  }
};

///
ApiProtocolHandler::ApiProtocolHandler(EWrapper& ewrapper) :
    impl_(new implementation(
        &ewrapper, new ContractDetailsEClientMock(ewrapper)))
{
    LOG(ERROR) << "**** Using mock EClient with EWrapper " << &ewrapper;
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



/// Must use a singleton otherwise weird zmq threading issues will occur.
service::ContractManager& getCm(int p1, int p2)
{
  static service::ContractManager* cm =
      new service::ContractManager(
          "tcp://127.0.0.1:" + boost::lexical_cast<string>(p1),
          "tcp://127.0.0.1:" + boost::lexical_cast<string>(p2));
  return *cm;
}

TEST(ClusterPrototype, RequestStockDetails)
{
  /// Server side
  LOG(INFO) << "Starting contract manager";

  unsigned int cm_endpoint = 26668;
  unsigned int cm_publish = 29999;

  ::ContractManager cmm;
  IBAPI::SocketInitiator* cm_server =
      startContractManager(cmm, cm_endpoint, cm_publish);


  /// Client side
  service::ContractManager& cm = getCm(cm_endpoint, cm_publish);

  LOG(INFO) << "ContractManager ready.";

  service::ContractManager::RequestId reqId(100);
  string symbol("GOOG");

  LOG(INFO) << "Sending request";

  service::AsyncContractDetailsEnd future =
      cm.requestStockContractDetails(reqId, symbol);

  LOG(INFO) << "Waiting for response";

  const p::ContractDetailsEnd& details_end = future->get(2000);

  EXPECT_TRUE(future->is_ready());

  LOG(INFO) << "Got response.  Looking up contract.";

  p::Contract contract;

  EXPECT_TRUE(cm.findContract(symbol + ".STK", &contract));
  EXPECT_EQ(30351181, contract.id());

  LOG(INFO) << "Found contract, conId = " << contract.id();
}

