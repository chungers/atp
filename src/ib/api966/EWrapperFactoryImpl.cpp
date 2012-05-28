
// api964
#include "ib/Application.hpp"
#include "ib/EWrapperFactory.hpp"
#include "ApiImpl.hpp"
#include "MarketEventDispatcher.hpp"


namespace ib {
namespace internal {


EWrapper* EWrapperFactory::getInstance(IBAPI::Application& app,
                                       EWrapperEventCollector& eventCollector,
                                       int clientId)
{
  return new MarketEventDispatcher(app, eventCollector, clientId);
}


} // internal
} // ib


