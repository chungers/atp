#ifndef IBAPI_INITIATOR_H_
#define IBAPI_INITIATOR_H_

#include <set>
#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>


#include <Shared/EWrapper.h>

#include "common.hpp"
#include "ib/Application.hpp"
#include "ib/Exceptions.hpp"
#include "ib/SocketConnector.hpp"
#include "ib/SessionSetting.hpp"



namespace IBAPI {


/// Models after Initiator in QuickFIX API:
/// https://github.com/lab616/third_party/blob/master/quickfix-1.13.3/src/C++/Initiator.h
class Initiator : NoCopyAndAssign {

 public:

  ~Initiator() {};

  virtual void start() throw ( ConfigError, RuntimeError ) {};
  virtual void block() throw ( ConfigError, RuntimeError ) {};

  virtual void stop(double timeout) {};
  virtual void stop(bool force = false) {};
  virtual bool isLoggedOn() { return false; };
};

} // namespace IBAPI

#endif // IBAPI_INITIATOR_H_
