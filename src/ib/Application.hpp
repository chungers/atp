#ifndef IBAPI_APPLICATION_H_
#define IBAPI_APPLICATION_H_

#include <string>
#include <set>

#include <boost/shared_ptr.hpp>

#include <Shared/EWrapper.h>


#include "log_levels.h"
#include "ib/internal.hpp"
#include "ib/Exceptions.hpp"
#include "ib/Message.hpp"
#include "ib/SessionID.hpp"

namespace ib {

typedef EWrapper* EWrapperPtr;
//typedef boost::shared_ptr<EWrapper> EWrapperPtr;

}

namespace IBAPI {


class ApiEventDispatcher;

///
/// This interface models after the Application interface in QuickFIX.
/// To receive messages from the IB gateway, an applicatio implements
/// this interface where the engine will call the methods on various events.
/// For more information,
/// see http://www.quickfixengine.org/quickfix/doc/html/application.html
class Application {

 public :

  virtual ~Application() {};

  virtual ApiEventDispatcher* GetApiEventDispatcher(const SessionID& sessionId)
  {
    return NULL;
  }

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

} // namespace IBAPI

#endif // IBAPI_APPLICATION_H_
