#ifndef IBAPI_APPLICATION_H_
#define IBAPI_APPLICATION_H_

#include "log_levels.h"
#include "ib/Exceptions.hpp"
#include "ib/Message.hpp"
#include "ib/SessionID.hpp"


namespace IBAPI {

///
/// This interface models after the Application interface in QuickFIX.  To receive messages from the
/// IB gateway, an applicatio implements this interface where the engine will call the methods on
/// various events.  For more information, see http://www.quickfixengine.org/quickfix/doc/html/application.html
class Application {

 public :
  virtual ~Application() {};

  virtual void onCreate( const SessionID& ) = 0;


  virtual void onLogon( const SessionID& ) = 0;


  virtual void onLogout( const SessionID& ) = 0;


  virtual void toAdmin( Message&, const SessionID& ) = 0;


  virtual void toApp( Message&, const SessionID& )
      throw( DoNotSend ) = 0;


  virtual void fromAdmin( const Message&, const SessionID& )
      throw( IncorrectDataFormat, IncorrectTagValue, RejectLogon ) = 0;

  /// Method called by the engine when events come from the gateway.  For the actual messages,
  /// @see Message
  virtual void fromApp( const Message&, const SessionID& )
      throw( IncorrectDataFormat, IncorrectTagValue, UnsupportedMessageType ) = 0;
};


class ApplicationBase : public Application {
 public:
  ApplicationBase() {}
  ~ApplicationBase() {}

  void onCreate( const SessionID& sessionId)
  {
    VLOG(VLOG_LEVEL_IBAPI_APPLICATION) << "onCreate(" << sessionId << ")" << std::endl;
  }


  void onLogon( const SessionID& sessionId)
  {
    VLOG(VLOG_LEVEL_IBAPI_APPLICATION) << "onLogon(" << sessionId << ")" << std::endl;
  }


  void onLogout( const SessionID& sessionId)
  {
    VLOG(VLOG_LEVEL_IBAPI_APPLICATION) << "onLogout(" << sessionId << ")" << std::endl;
  }


  void toAdmin( Message& message, const SessionID& sessionId)
  {
    VLOG(VLOG_LEVEL_IBAPI_APPLICATION) << "toAdmin(" << sessionId << ")" << std::endl;
  }

  void toApp( Message& message, const SessionID& sessionId) throw( DoNotSend )
  {
    VLOG(VLOG_LEVEL_IBAPI_APPLICATION) << "toApp(" << sessionId << ")" << std::endl;
  }

  void fromAdmin( const Message& message, const SessionID& sessionId)
      throw( IncorrectDataFormat, IncorrectTagValue, RejectLogon )
  {
    VLOG(VLOG_LEVEL_IBAPI_APPLICATION) << "fromAdmin(" << sessionId << ")" << std::endl;
  }

  void fromApp( const Message& message, const SessionID& sessionId)
      throw( IncorrectDataFormat, IncorrectTagValue, UnsupportedMessageType )
  {
    VLOG(VLOG_LEVEL_IBAPI_APPLICATION) << "fromApp(" << sessionId << ")" << std::endl;
  }

};

} // namespace IBAPI

#endif // IBAPI_APPLICATION_H_
