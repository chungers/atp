
// api964
#include "ib/Application.hpp"
#include "ib/EWrapperFactory.hpp"
#include "ApiImpl.hpp"
#include "EventDispatcher.hpp"


namespace ib {
namespace internal {


EWrapper* EWrapperFactory::getInstance(IBAPI::Application& app,
                                       EWrapperEventSink& eventSink,
                                       int clientId)
{
  return new EventDispatcher(app, eventSink, clientId);
}


} // internal
} // ib


