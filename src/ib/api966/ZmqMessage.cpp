
// api966
#include <map>
#include <boost/bind.hpp>
#include <boost/function.hpp>

#include "ib/ZmqMessage.hpp"
#include "zmq/ZmqUtils.hpp"

#include "ApiMessages.hpp"


namespace ib {
namespace internal {

typedef std::map< std::string, ZmqMessagePtr > CallApiHandlerMap;

const CallApiHandlerMap& _handlerMap()
{
  using namespace IBAPI::V966;

  static CallApiHandlerMap handlerMap;

  {
    ZmqMessagePtr p(new RequestMarketData());
    handlerMap[ (*p)->key() ] = p;
  }
  {
    ZmqMessagePtr p(new CancelMarketData());
    handlerMap[ (*p)->key() ] = p;
  }
  {
    ZmqMessagePtr p(new RequestMarketDepth());
    handlerMap[ (*p)->key() ] = p;
  }
  {
    ZmqMessagePtr p(new CancelMarketDepth());
    handlerMap[ (*p)->key() ] = p;
  }
  {
    ZmqMessagePtr p(new CancelOrder());
    handlerMap[ (*p)->key() ] = p;
  }
  {
    ZmqMessagePtr p(new MarketOrder());
    handlerMap[ (*p)->key() ] = p;
  }
  {
    ZmqMessagePtr p(new LimitOrder());
    handlerMap[ (*p)->key() ] = p;
  }
  {
    ZmqMessagePtr p(new StopLimitOrder());
    handlerMap[ (*p)->key() ] = p;
  }
  return handlerMap;
}


void ZmqMessage::createMessage(const std::string& msgKey, ZmqMessagePtr& ptr)
{
  ZmqMessagePtr empty;
  const CallApiHandlerMap& map = _handlerMap();
  if (map.find(msgKey) == map.end()) {
    ptr = empty;
    return;
  } else {
    ptr = map.find(msgKey)->second;
  }
};


} // internal
} // ib


