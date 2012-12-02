#ifndef MAIN_FH_H_
#define MAIN_FH_H_


#include <string>

#include <boost/assign.hpp>
#include <boost/unordered_set.hpp>

#include <glog/logging.h>

#include "ib/ApplicationBase.hpp"
#include "ib/SocketInitiator.hpp"
#include "ib/MarketEventDispatcher.hpp"

using boost::unordered_set;
using std::string;
using IBAPI::ApiEventDispatcher;
using IBAPI::SessionID;

/// format:  {session_id}={gateway_ip_port}@{reactor_endpoint}
const std::string CONNECTOR_SPECS =
    "100=127.0.0.1:4001@tcp://127.0.0.1:6666";

const std::string OUTBOUND_ENDPOINTS = "0=tcp://127.0.0.1:7777";

const proto::ib::RequestContractDetails REQUEST_CONTRACT_DETAILS;
const proto::ib::RequestMarketData REQUEST_MARKET_DATA;
const proto::ib::RequestMarketDepth REQUEST_MARKET_DEPTH;
const proto::ib::CancelMarketData CANCEL_MARKET_DATA;
const proto::ib::CancelMarketDepth CANCEL_MARKET_DEPTH;

// Firehose only supports messages related to market data.
const unordered_set<string> FIREHOSE_VALID_MESSAGES_ =
    boost::assign::list_of
    (REQUEST_MARKET_DATA.GetTypeName())
    (REQUEST_MARKET_DEPTH.GetTypeName())
    (CANCEL_MARKET_DATA.GetTypeName())
    (CANCEL_MARKET_DEPTH.GetTypeName())
    (REQUEST_CONTRACT_DETAILS.GetTypeName())
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
