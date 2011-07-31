#ifndef IB_APPLICATION_H_
#define IB_APPLICATION_H_

namespace IBAPI {

typedef unsigned int SessionID;
struct Message {

};

struct NextOrderIdMessage : Message {
  int nextOrderId;
};

struct HeartBeatMessage : Message {
  long currentTime;
};

struct DoNotSend {

};

struct IncorrectDataFormat {

};

struct IncorrectTagValue {

};

struct UnsupportedMessageType {

};

struct RejectLogon {

};

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
  virtual void fromApp( const Message&, const SessionID& )
      throw( IncorrectDataFormat, IncorrectTagValue, UnsupportedMessageType ) = 0;
};
} // namespace IBAPI

#endif // IB_APPLICATION_H_
