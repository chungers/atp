
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

#include "common/time_utils.hpp"

#include "main/global_defaults.hpp"
#include "service/ContractManager.hpp"
#include "platform/contract_symbol.hpp"


using std::string;
using namespace boost::gregorian;


namespace service = atp::service;
namespace p = proto::ib;


/// Must use a singleton otherwise weird zmq threading issues will occur.
service::ContractManager& getCm()
{
  static service::ContractManager* cm =
      new service::ContractManager(atp::global::CM_CONTROLLER_ENDPOINT,
                                   atp::global::CM_OUTBOUND_ENDPOINT);
  return *cm;
}

TEST(ContractManagerPrototype, RequestStockDetails)
{
  service::ContractManager& cm = getCm();

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

TEST(ContractManagerPrototype, RequestOptionChain)
{
  service::ContractManager& cm = getCm();

  LOG(INFO) << "ContractManager ready.";

  service::ContractManager::RequestId reqId(2000);
  string symbol("AAPL");

  LOG(INFO) << "Sending request";
  service::AsyncContractDetailsEnd future =
      cm.requestStockContractDetails(reqId, symbol);

  LOG(INFO) << "Waiting for response";

  const p::ContractDetailsEnd& details_end = future->get(2000);

  EXPECT_TRUE(future->is_ready());
  p::Contract contract;
  EXPECT_TRUE(cm.findContract(symbol + ".STK", &contract));


  // Now request option chain for the same symbol
  service::AsyncContractDetailsEnd future2 =
      cm.requestOptionChain(reqId + 1, symbol);

  LOG(INFO) << "Waiting for response2";

  const p::ContractDetailsEnd& details_end2 = future2->get();

  EXPECT_TRUE(future2->is_ready());

  p::Contract aapl_put;
  EXPECT_TRUE(cm.findContract("AAPL.OPT.20130104.510.P", &aapl_put));

  EXPECT_TRUE(
      cm.findOptionContract("AAPL", date(2013, Jan, 4), 510.,
                            service::ContractManager::PutOption,
                            &aapl_put));

}
