#ifndef IBAPI_EWRAPPER_FACTORY_H_
#define IBAPI_EWRAPPER_FACTORY_H_

#include <boost/shared_ptr.hpp>
#include <Shared/EWrapper.h>

#include "ib/Application.hpp"
#include "ib/SocketConnector.hpp"


namespace ib {
namespace internal {

/// Simple factory class for getting a EWrapper that can receive events
/// from IB's API.  This is basically an interface, with a static function
/// for obtaining the actual factory implementation.
///
/// Different implementations (e.g. for testing, different api version) will
/// provide the implementation (.cpp) of this interface and implement the
/// static factory method.  At compile /link time, the actual api version
/// (or as in the case of testing), is specified and the .cpp implementation
/// file is compiled and linked.
class EWrapperFactory {

 public:
  ~EWrapperFactory() {}

  virtual EWrapper* getImpl(IBAPI::Application& app,
                            IBAPI::SocketConnector::Strategy& strategy,
                            int clientId = 0) = 0;

  /// Returns a shared pointer to the factory.
  static boost::shared_ptr<EWrapperFactory> getInstance();
};



} // internal
} // ib
#endif IBAPI_EWRAPPER_FACTORY_H_
