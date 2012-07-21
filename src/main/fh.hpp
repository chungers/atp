#ifndef MAIN_FH_H_
#define MAIN_FH_H_

#include <map>
#include <set>
#include <string>

#include <boost/assign.hpp>

#include "ib/ApplicationBase.hpp"
#include "ib/SocketInitiator.hpp"
#include "ib/MarketEventDispatcher.hpp"

using std::map;
using std::set;
using std::string;
using IBAPI::ApiEventDispatcher;
using IBAPI::SessionID;

/// format:  {session_id}={gateway_ip_port}@{reactor_endpoint}
const std::string CONNECTOR_SPECS =
    "100=127.0.0.1:4001@tcp://127.0.0.1:6666";

const std::string OUTBOUND_ENDPOINTS = "0=tcp://127.0.0.1:7777";

// Firehose only supports messages related to market data.
const set<string> FIREHOSE_VALID_MESSAGES_ =
               boost::assign::list_of
               ("IBAPI.FEED.RequestMarketData")
               ("IBAPI.FEED.CancelMarketData")
               ("IBAPI.FEED.RequestMarketDepth")
               ("IBAPI.FEED.CancelMarketDepth")
               ;



class Firehose : public IBAPI::ApplicationBase
{
 public:

  Firehose() {}
  ~Firehose() {}

  virtual bool IsMessageSupported(const std::string& key)
  {
    return FIREHOSE_VALID_MESSAGES_.find(key) != FIREHOSE_VALID_MESSAGES_.end();
  }

  virtual ApiEventDispatcher* GetApiEventDispatcher(const SessionID& sessionId)
  {
    return new ib::internal::MarketEventDispatcher(*this, sessionId);
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


#endif //MAIN_FH_H_
