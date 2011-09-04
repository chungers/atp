#ifndef IBAPI_MESSAGE_H_
#define IBAPI_MESSAGE_H_

#include <quickfix/FieldMap.h>

#include "ib/ib_common.hpp"
#include "ib/IBAPIFields.hpp"
#include "ib/IBAPIValues.hpp"

using namespace FIX;

namespace IBAPI {


class Header : public FIX::FieldMap {
 public:
  Header(const std::string& msgType)
  {
    setField(IBAPI::MsgType(msgType));
  }

  FIELD_SET(*this, IBAPI::MsgType);
  FIELD_SET(*this, IBAPI::SendingTime);
};

class Trailer : public FIX::FieldMap {
 public:
  Trailer()
  {
    std::string time;
    now_micros(&time);
    setField(IBAPI::Ext_SendingTimeMicros(time));
  }

  FIELD_SET(*this, IBAPI::Ext_SendingTimeMicros);
  FIELD_SET(*this, IBAPI::Ext_OrigSendingTimeMicros);
};

class Message : public FIX::FieldMap
{
 public:
  Message(const std::string& msgType) : header_(msgType)
  {
  }

  const Header& getHeader()
  { return header_; }

  const Trailer& getTrailer()
  { return trailer_; }

 protected:
  mutable Header header_;
  mutable Trailer trailer_;
};

} // namespace IBAPI

#endif // IBAPI_MESSAGE_H_
