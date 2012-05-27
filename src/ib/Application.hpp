#ifndef IBAPI_APPLICATION_H_
#define IBAPI_APPLICATION_H_

#include <string>
#include <set>

#include "log_levels.h"
#include "ib/Exceptions.hpp"
#include "ib/Message.hpp"
#include "ib/SessionID.hpp"


namespace IBAPI {

///
/// This interface models after the Application interface in QuickFIX.
/// To receive messages from the IB gateway, an applicatio implements
/// this interface where the engine will call the methods on various events.
/// For more information,
/// see http://www.quickfixengine.org/quickfix/doc/html/application.html
class Application {

 public :

  virtual ~Application() {};

  virtual bool IsMessageSupported(const std::string& key)
  {
    return true;
  }

  virtual void onCreate( const SessionID& ) = 0;


  virtual void onLogon( const SessionID& ) = 0;


  virtual void onLogout( const SessionID& ) = 0;


  virtual void toAdmin( Message&, const SessionID& ) = 0;


  virtual void toApp( Message&, const SessionID& )
      throw( DoNotSend ) = 0;


  virtual void fromAdmin( const Message&, const SessionID& )
      throw( IncorrectDataFormat, IncorrectTagValue, RejectLogon ) = 0;

  /// Method called by the engine when events come from the gateway.
  /// For the actual messages, @see Message
  virtual void fromApp( const Message&, const SessionID& )
      throw( IncorrectDataFormat,
             IncorrectTagValue,
             UnsupportedMessageType ) = 0;
};


class ApplicationBase : public Application {
 public:
  ApplicationBase() {}
  ~ApplicationBase() {}

  void onCreate( const SessionID& sessionId)
  {
    IBAPI_APPLICATION_LOGGER << "onCreate(" << sessionId << ")";
  }


  void onLogon( const SessionID& sessionId)
  {
    IBAPI_APPLICATION_LOGGER << "onLogon(" << sessionId << ")";
  }


  void onLogout( const SessionID& sessionId)
  {
    IBAPI_APPLICATION_LOGGER << "onLogout(" << sessionId << ")";
  }


  void toAdmin( Message& message, const SessionID& sessionId)
  {
    IBAPI_APPLICATION_LOGGER << "toAdmin(" << sessionId
                             << "," << &message
                             << ")";
  }

  void toApp( Message& message, const SessionID& sessionId) throw( DoNotSend )
  {
    IBAPI_APPLICATION_LOGGER << "toApp(" << sessionId
                             << "," << &message
                             << ")";
  }

  void fromAdmin( const Message& message, const SessionID& sessionId)
      throw( IncorrectDataFormat, IncorrectTagValue, RejectLogon )
  {
    IBAPI_APPLICATION_LOGGER << "fromAdmin(" << sessionId
                             << "," << &message
                             << ")";
  }

  void fromApp( const Message& message, const SessionID& sessionId)
      throw( IncorrectDataFormat, IncorrectTagValue, UnsupportedMessageType )
  {
    IBAPI_APPLICATION_LOGGER << "fromApp(" << sessionId
                             << "," << &message
                             << ")";
  }

};

} // namespace IBAPI

#endif // IBAPI_APPLICATION_H_
