#ifndef IB_INTERNAL_MARKET_EVENT_DISPATCHER_H_
#define IB_INTERNAL_MARKET_EVENT_DISPATCHER_H_

#include <string>
#include "common.hpp"
#include "ib/ApiEventDispatcher.hpp"


namespace ib {
namespace internal {

class MarketEventDispatcher : public IBAPI::ApiEventDispatcher
{
 public:
  MarketEventDispatcher(IBAPI::Application& app,
                        const IBAPI::SessionID& sessionId);

  ~MarketEventDispatcher();


  // Implementation in version specific impl directory.
  virtual EWrapper* GetEWrapper();

  template <typename T>
  void publish(TickerId tickerId, TickType tickType, const T& value,
               TimeTracking& timed);

  void publishDepth(TickerId tickerId, int side, int level, int operation,
                    double price, int size,
                    TimeTracking& timed,
                    const std::string& mm = "L1");



};


template <>
inline void MarketEventDispatcher::publish(TickerId tickerId, TickType tickType,
                                           const double& value,
                                           TimeTracking& timed)
{
  return MarketEventDispatcher::publish<double>(tickerId, tickType,
                                                value, timed);
}

template <>
inline void MarketEventDispatcher::publish(TickerId tickerId, TickType tickType,
                                           const int& value,
                                           TimeTracking& timed)
{
  return MarketEventDispatcher::publish<int>(tickerId, tickType,
                                             value, timed);
}

template <>
inline void MarketEventDispatcher::publish(TickerId tickerId, TickType tickType,
                                           const std::string& value,
                                           TimeTracking& timed)
{
  return MarketEventDispatcher::publish<std::string>(tickerId, tickType,
                                                     value, timed);
}


} // internal
} // ib


#endif // IB_INTERNAL_MARKET_EVENT_DISPATCHER_H_
