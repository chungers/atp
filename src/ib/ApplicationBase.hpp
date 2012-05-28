#ifndef IBAPI_APPLICATION_BASE_H_
#define IBAPI_APPLICATION_BASE_H_

#include <string>
#include <set>

#include "ib/Application.hpp"


namespace IBAPI {


class ApplicationBase : public Application {
 public:
  ApplicationBase() {}
  ~ApplicationBase() {}

  virtual void onCreate(const SessionID& sessionId)
  {
    IBAPI_APPLICATION_LOGGER << "onCreate(" << sessionId << ")";
  }


  virtual void onLogon(const SessionID& sessionId)
  {
    IBAPI_APPLICATION_LOGGER << "onLogon(" << sessionId << ")";
  }


  virtual void onLogout(const SessionID& sessionId)
  {
    IBAPI_APPLICATION_LOGGER << "onLogout(" << sessionId << ")";
  }


  virtual void toAdmin(Message& message, const SessionID& sessionId)
  {
    IBAPI_APPLICATION_LOGGER << "toAdmin(" << sessionId
                             << "," << &message
                             << ")";
  }

  virtual void toApp(Message& message, const SessionID& sessionId)
      throw( DoNotSend )
  {
    IBAPI_APPLICATION_LOGGER << "toApp(" << sessionId
                             << "," << &message
                             << ")";
  }

  virtual void fromAdmin( const Message& message, const SessionID& sessionId)
      throw( IncorrectDataFormat, IncorrectTagValue, RejectLogon )
  {
    IBAPI_APPLICATION_LOGGER << "fromAdmin(" << sessionId
                             << "," << &message
                             << ")";
  }

  virtual void fromApp( const Message& message, const SessionID& sessionId)
      throw( IncorrectDataFormat, IncorrectTagValue, UnsupportedMessageType )
  {
    IBAPI_APPLICATION_LOGGER << "fromApp(" << sessionId
                             << "," << &message
                             << ")";
  }

};

} // namespace IBAPI

#endif // IBAPI_APPLICATION_BASE_H_
