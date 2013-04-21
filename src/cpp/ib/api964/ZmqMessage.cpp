
// api964
#include "ApiMessages.hpp"


namespace ib {
namespace internal {

using IBAPI::V964::CancelMarketDataRequest;
using IBAPI::V964::MarketDataRequest;

using IBAPI::V964::CancelMarketDepthRequest;
using IBAPI::V964::MarketDepthRequest;

using IBAPI::V964::CancelMarketOhlcRequest;
using IBAPI::V964::MarketOhlcRequest;


void ZmqMessage::createMessage(const std::string& msgKey, ZmqMessagePtr& ptr)
{
    if (msgKey == "V964.MarketDataRequest") {
      ptr = ZmqMessagePtr(new MarketDataRequest());
    }
    else if (msgKey == "V964.CancelMarketDataRequest") {
      ptr = ZmqMessagePtr(new CancelMarketDataRequest());
    }
    else if (msgKey == "V964.MarketDepthRequest") {
      ptr = ZmqMessagePtr(new MarketDepthRequest());
    }
    else if (msgKey == "V964.CancelMarketDepthRequest") {
      ptr =  ZmqMessagePtr(new CancelMarketDepthRequest());
    }
    else if (msgKey == "V964.MarketOhlcRequest") {
      ptr = ZmqMessagePtr(new MarketOhlcRequest());
    }
    else if (msgKey == "V964.CancelMarketOhlcRequest") {
      ptr = ZmqMessagePtr(new CancelMarketOhlcRequest());
    } else {
      ZmqMessagePtr empty;
      ptr = empty;
    }
};


} // internal
} // ib


