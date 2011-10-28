#ifndef IBAPI_EWRAPPER_FACTORY_H_
#define IBAPI_EWRAPPER_FACTORY_H_

#include <boost/shared_ptr.hpp>
#include <Shared/EWrapper.h>
#include <zmq.hpp>

#include "ib/Application.hpp"
#include "ib/SocketConnector.hpp"


namespace ib {
namespace internal {

class EWrapperEventCollector
{
 public:
  ~EWrapperEventCollector() {}

  /// Returns the ZMQ socket that will be written to.
  virtual zmq::socket_t* getOutboundSocket() = 0;

  // TODO: add a << operator
};


/// Simple factory class for getting a EWrapper that can receive events
/// from IB's API.  This is basically an interface, with a static function
/// for obtaining the actual factory implementation.
///
/// Different implementations (e.g. for testing, different api version) will
/// provide the implementation (.cpp) of this interface and implement the
/// static factory method.  At compile /link time, the actual api version
/// (or as in the case of testing), is specified and the .cpp implementation
/// file is compiled and linked.
class EWrapperFactory
{

 private:
  EWrapperFactory() {}
  ~EWrapperFactory() {}

 public:
  static EWrapper* getInstance(IBAPI::Application& app,
                               EWrapperEventCollector& eventCollector,
                               int clientId = 0);
};



} // internal
} // ib
#endif IBAPI_EWRAPPER_FACTORY_H_
