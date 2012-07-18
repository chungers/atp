#ifndef MAIN_EM_H_
#define MAIN_EM_H_

#include <map>
#include <set>
#include <string>

#include <boost/assign.hpp>

#include "ib/ApplicationBase.hpp"
#include "ib/SocketInitiator.hpp"
#include "ib/OrderEventDispatcher.hpp"

using std::map;
using std::set;
using std::string;
using IBAPI::ApiEventDispatcher;
using IBAPI::SessionID;


/// format:  {session_id}={gateway_ip_port}@{reactor_endpoint}
const string CONNECTOR_SPECS =
    "200=127.0.0.1:4001@tcp://127.0.0.1:6667";

const string OUTBOUND_ENDPOINTS =
    "0=tcp://127.0.0.1:8888";

// ExecutionManager only supports messages related to orders
const set<string> EM_VALID_MESSAGES_ =
               boost::assign::list_of
               ("IBAPI.EXEC.CancelOrder")
               ("IBAPI.EXEC.MarketOrder")
               ("IBAPI.EXEC.LimitOrder")
               ("IBAPI.EXEC.StopLimitOrder")
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
