#ifndef MAIN_CM_H_
#define MAIN_CM_H_

#include <string>

#include <boost/assign.hpp>
#include <boost/unordered_set.hpp>

#include "ib/ApplicationBase.hpp"
#include "ib/SocketInitiator.hpp"
#include "ib/ContractEventDispatcher.hpp"

using boost::unordered_set;
using std::string;
using IBAPI::ApiEventDispatcher;
using IBAPI::SessionID;


/// format:  {session_id}={gateway_ip_port}@{reactor_endpoint}
const string CONNECTOR_SPECS =
    "300=127.0.0.1:4001@tcp://127.0.0.1:6668";

const string OUTBOUND_ENDPOINTS =
    "0=tcp://127.0.0.1:9999";

const proto::ib::ContractDetailsResponse CONTRACT_DETAILS_RESP;
const proto::ib::ContractDetailsEnd CONTRACT_DETAILS_END;

// ContractManager only supports messages related to orders
const unordered_set<string> CM_VALID_MESSAGES_ =
    boost::assign::list_of
    (CONTRACT_DETAILS_RESP.GetTypeName())
    (CONTRACT_DETAILS_END.GetTypeName())
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
