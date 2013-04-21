#ifndef MAIN_EM_H_
#define MAIN_EM_H_

#include <string>

#include <boost/assign.hpp>
#include <boost/unordered_set.hpp>

#include "ib/ApplicationBase.hpp"
#include "ib/SocketInitiator.hpp"
#include "ib/OrderEventDispatcher.hpp"

#include "main/global_defaults.hpp"


using boost::unordered_set;
using std::string;
using IBAPI::ApiEventDispatcher;
using IBAPI::SessionID;

const proto::ib::CancelOrder CANCEL_ORDER;
const proto::ib::MarketOrder MARKET_ORDER;
const proto::ib::LimitOrder LIMIT_ORDER;
const proto::ib::StopLimitOrder STOP_LIMIT_ORDER;

// ExecutionManager only supports messages related to orders
const unordered_set<string> EM_VALID_MESSAGES_ =
    boost::assign::list_of
    (CANCEL_ORDER.GetTypeName())
    (MARKET_ORDER.GetTypeName())
    (LIMIT_ORDER.GetTypeName())
    (STOP_LIMIT_ORDER.GetTypeName())
    ;

class ExecutionManager : public IBAPI::ApplicationBase
{
 public:

  ExecutionManager() {}
  ~ExecutionManager() {}


  virtual bool IsMessageSupported(const string& key)
  {
    return EM_VALID_MESSAGES_.find(key) != EM_VALID_MESSAGES_.end();
  }

  virtual ApiEventDispatcher* GetApiEventDispatcher(const SessionID& sessionId)
  {
    return new ib::internal::OrderEventDispatcher(*this, sessionId);
  }

  void onLogon(const SessionID& sessionId)
  {
    LOG(INFO) << "Session " << sessionId << " logged on.";
  }

  void onLogout(const SessionID& sessionId)
  {
    LOG(INFO) << "Session " << sessionId << " logged off.";
  }

};


#endif //MAIN_EM_H_
