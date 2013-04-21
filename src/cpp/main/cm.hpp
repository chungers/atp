#ifndef MAIN_CM_H_
#define MAIN_CM_H_

#include <string>

#include <boost/assign.hpp>
#include <boost/unordered_set.hpp>

#include "ib/ApplicationBase.hpp"
#include "ib/SocketInitiator.hpp"
#include "ib/ContractEventDispatcher.hpp"

#include "main/global_defaults.hpp"


using boost::unordered_set;
using std::string;
using IBAPI::ApiEventDispatcher;
using IBAPI::SessionID;

const proto::ib::RequestContractDetails REQUEST_CONTRACT_DETAILS;

// ContractManager only supports messages related to orders
const unordered_set<string> CM_VALID_MESSAGES_ =
    boost::assign::list_of
    (REQUEST_CONTRACT_DETAILS.GetTypeName())
    ;

class ContractManager : public IBAPI::ApplicationBase
{
 public:

  ContractManager() {}
  ~ContractManager() {}


  virtual bool IsMessageSupported(const string& key)
  {
    return CM_VALID_MESSAGES_.find(key) != CM_VALID_MESSAGES_.end();
  }

  virtual ApiEventDispatcher* GetApiEventDispatcher(const SessionID& sessionId)
  {
    return new ib::internal::ContractEventDispatcher(*this, sessionId);
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


#endif //MAIN_CM_H_
