
// api964
#include "ib/Application.hpp"
#include "ib/EWrapperFactory.hpp"
#include "ApiImpl.hpp"
#include "EventDispatcher.hpp"


namespace ib {
namespace internal {


class EWrapperFactoryImpl : public EWrapperFactory
{

 public:
  EWrapperFactoryImpl()
  {
    LOG(INFO) << "API964 EWrapperFactory" << std::endl;
  }

  ~EWrapperFactoryImpl()
  {
  }

  /// Implements EWrapperFactory
  EWrapper* getImpl(IBAPI::Application& app,
                    EWrapperEventSink& eventSink,
                    int clientId)
  {
    return new EventDispatcher(app, eventSink, clientId);
  }

};

boost::shared_ptr<EWrapperFactory> EWrapperFactory::getInstance()
{
  return boost::shared_ptr<EWrapperFactory>(new EWrapperFactoryImpl());
}


} // internal
} // ib


