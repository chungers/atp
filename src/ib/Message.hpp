#ifndef IBAPI_MESSAGE_H_
#define IBAPI_MESSAGE_H_

#include <quickfix/FieldMap.h>
#include <quickfix/Fields.h>

#include "ib/ib_common.hpp"
#include "ib/IBAPIFields.hpp"
#include "ib/IBAPIValues.hpp"

using namespace FIX;

namespace IBAPI {

typedef std::string MsgType;
typedef std::string MsgKey;

class Header : public FIX::FieldMap {
 public:
  Header() {}

  Header(const MsgType& msgType,
         const MsgKey& msgKey)
  {
    setField(FIX::BeginString(msgKey));
    setField(FIX::MsgType(msgType));
  }

  Header(const Header& copy) : FIX::FieldMap(copy)
  {
  }

  FIELD_SET(*this, FIX::MsgType);
  FIELD_SET(*this, FIX::BeginString);
  FIELD_SET(*this, FIX::SendingTime);
};

class Trailer : public FIX::FieldMap {
 public:
  Trailer()
  {
    std::string time;
    now_micros(&time);
    setField(IBAPI::Ext_SendingTimeMicros(time));
  }

  Trailer(const Trailer& copy) : FIX::FieldMap(copy)
  {
  }

  FIELD_SET(*this, IBAPI::Ext_SendingTimeMicros);
  FIELD_SET(*this, IBAPI::Ext_OrigSendingTimeMicros);
};

class Message : public FIX::FieldMap
{
 public:
  Message() {}
  Message(const MsgType& msgType,
          const MsgKey& msgKey) : header_(msgType, msgKey)
  {
  }

  Message(const Message& copy) :
      FIX::FieldMap(copy),
      header_(copy.header_), trailer_(copy.trailer_)
  {

  }

  Header& getHeader()
  { return header_; }

  Trailer& getTrailer()
  { return trailer_; }

 protected:
  mutable Header header_;
  mutable Trailer trailer_;
};

} // namespace IBAPI

#endif // IBAPI_MESSAGE_H_
