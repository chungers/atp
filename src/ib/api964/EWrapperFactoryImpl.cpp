
// api964
#include "ib/Application.hpp"
#include "ib/EWrapperFactory.hpp"
#include "ib/SocketConnector.hpp"
#include "ApiImpl.hpp"
#include "EventDispatcher.hpp"


namespace ib {
namespace internal {


class EWrapperFactoryImpl : public EWrapperFactory {

 public:
  EWrapperFactoryImpl() {
    LOG(INFO) << "EWrapperFactory start." << std::endl;
  }
  ~EWrapperFactoryImpl() {
    LOG(INFO) << "EWrapperFactory done." << std::endl;
  }

  /// Implements EWrapperFactory
  EWrapper* getImpl(IBAPI::Application& app, IBAPI::SocketConnector::Strategy& strategy) {
    return new EventDispatcher(app, strategy);
  }

};

boost::shared_ptr<EWrapperFactory> EWrapperFactory::getInstance() {
  return boost::shared_ptr<EWrapperFactory>(new EWrapperFactoryImpl());
}


} // internal
} // ib


