#ifndef IBAPI_INITIATOR_H_
#define IBAPI_INITIATOR_H_

#include "common.hpp"
#include "ib/Application.hpp"
#include "ib/Exceptions.hpp"
#include "ib/SocketConnector.hpp"
#include "ib/SessionSetting.hpp"


namespace IBAPI {


/// Models after Initiator in QuickFIX API: http://goo.gl/icWxF
class Initiator : NoCopyAndAssign
{

 public:

  ~Initiator() {};

  virtual void start(bool connect=true) throw ( ConfigError, RuntimeError ) = 0;
  virtual void block() throw ( ConfigError, RuntimeError ) = 0;

  virtual void stop(bool force = false) = 0;
  virtual bool isLoggedOn() = 0;

};

} // namespace IBAPI

#endif // IBAPI_INITIATOR_H_
