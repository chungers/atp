
#include <algorithm>
#include <map>
#include <set>
#include <vector>
#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include <glog/logging.h>

#include <boost/shared_ptr.hpp>

#include "ib/Application.hpp"
#include "ib/EWrapperFactory.hpp"

#include "ib/TestHarness.hpp"


#include "ib/api964/EventDispatcher.hpp"

namespace ib {
namespace internal {


class TestEWrapper : public EventDispatcher, public TestHarness {
 public:
  TestEWrapper(IBAPI::Application& app, EWrapperEventCollector& eventCollector,
               int clientId)
      : EventDispatcher(app, eventCollector, clientId),
        TestHarness()
  {
    LOG(INFO) << "Initialized LoggingEWrapper." << std:: endl;
  }

 public:

  // @Override
  void nextValidId(OrderId orderId) {
    EventDispatcher::nextValidId(orderId);
    incr(NEXT_VALID_ID);

    zmq::socket_t* eventCollector = getOutboundSocket();
    EXPECT_TRUE(eventCollector != NULL);

    LOG(INFO) << "Event eventCollector: " << eventCollector << std::endl;
  }

  // @Override
  void tickPrice(TickerId tickerId, TickType tickType, double price, int canAutoExecute) {
    EventDispatcher::tickPrice(tickerId, tickType, price, canAutoExecute);
    incr(TICK_PRICE);
    seen(tickerId);
  }

  // @Override
  void tickSize(TickerId tickerId, TickType tickType, int size) {
    EventDispatcher::tickSize(tickerId, tickType, size);
    incr(TICK_SIZE);
    seen(tickerId);
  }

  // @Override
  void tickGeneric(TickerId tickerId, TickType tickType, double value) {
    EventDispatcher::tickGeneric(tickerId, tickType, value);
    incr(TICK_GENERIC);
    seen(tickerId);
  }

  // @Override
  void tickOptionComputation(TickerId tickerId, TickType tickType, double impliedVol, double delta,
                             double optPrice, double pvDividend,
                             double gamma, double vega, double theta, double undPrice) {
    EventDispatcher::tickOptionComputation(tickerId, tickType, impliedVol, delta,
                                           optPrice, pvDividend, gamma, vega, theta, undPrice);
    incr(TICK_OPTION_COMPUTATION);
    seen(tickerId);
  }

  // @Override
  void updateMktDepth(TickerId id, int position, int operation, int side,
                      double price, int size) {
    EventDispatcher::updateMktDepth(id, position, operation, side, price, size);
    incr(UPDATE_MKT_DEPTH);
    seen(id);
   }

  // @Override
  void contractDetails( int reqId, const ContractDetails& contractDetails) {
    EventDispatcher::contractDetails(reqId, contractDetails);
    incr(CONTRACT_DETAILS);
    seen(reqId);
    if (optionChain_) {
      Contract c = contractDetails.summary;
      optionChain_->push_back(c);
    }
  }

  // @Override
  void contractDetailsEnd( int reqId) {
    EventDispatcher::contractDetailsEnd(reqId);
    incr(CONTRACT_DETAILS_END);
    seen(reqId);
  }

  // @Override
  void currentTime(long time) {
    incr(CURRENT_TIME);
  }
};

using ib::internal::EWrapperEventCollector;

EWrapper*
EWrapperFactory::getInstance(IBAPI::Application& app,
                             EWrapperEventCollector& eventCollector,
                             int clientId)
{
  LOG(INFO) << "Getting test EWrapper instance." << std::endl;
  return new ib::internal::TestEWrapper(app, eventCollector, clientId);
}



} // internal
} // ib


