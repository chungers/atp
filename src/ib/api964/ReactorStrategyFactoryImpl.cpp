
// api964
#include "ib/ReactorStrategyFactory.hpp"
#include "ApiMessages.hpp"


namespace ib {
namespace internal {

using ib::internal::ReactorStrategy;
using ib::internal::ReactorStrategyFactory;

using IBAPI::V964::CancelMarketDataRequest;
using IBAPI::V964::MarketDataRequest;

using IBAPI::V964::CancelMarketDepthRequest;
using IBAPI::V964::MarketDepthRequest;

using IBAPI::V964::CancelMarketOhlcRequest;
using IBAPI::V964::MarketOhlcRequest;



class ReactorStrategyImpl : public ReactorStrategy
{
  virtual bool handleInboundMessage(Message& message,
                                    EClientPtr eclient)
  {
    FIX::MsgType msgType;
    message.getHeader().get(msgType);
    if (msgType.getString() == "MarketDataRequest") {
      MarketDataRequest copy(message);
      return copy.callApi(eclient);
    }
    else if (msgType.getString() == "CancelMarketDataRequest") {
      CancelMarketDataRequest copy(message);
      return copy.callApi(eclient);
    }
    else if (msgType.getString() == "MarketDepthRequest") {
      MarketDepthRequest copy(message);
      return copy.callApi(eclient);
    }
    else if (msgType.getString() == "CancelMarketDepthRequest") {
      CancelMarketDepthRequest copy(message);
      return copy.callApi(eclient);
    }
    else if (msgType.getString() == "MarketOhlcRequest") {
      MarketOhlcRequest copy(message);
      return copy.callApi(eclient);
    }
    else if (msgType.getString() == "CancelMarketOhlcRequest") {
      CancelMarketOhlcRequest copy(message);
      return copy.callApi(eclient);
    }

    return false;
  }
};

ReactorStrategy* ReactorStrategyFactory::getInstance()
{
  return new ReactorStrategyImpl();
};

} // internal
} // ib


