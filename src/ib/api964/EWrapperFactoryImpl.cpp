
// api964
#include "ib/Application.hpp"
#include "ib/EWrapperFactory.hpp"
#include "ApiImpl.hpp"
#include "EventDispatcher.hpp"


namespace ib {
namespace internal {


EWrapper* EWrapperFactory::getInstance(IBAPI::Application& app,
                                       EWrapperEventCollector& eventCollector,
                                       int clientId)
{
  return new EventDispatcher(app, eventCollector, clientId);
}


} // internal
} // ib


