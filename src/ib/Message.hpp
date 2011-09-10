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
  Header() {}

  Header(const std::string& version,
         const std::string& msgType)
  {
    setField(IBAPI::MsgType(msgType));
    setField(IBAPI::BeginString(version));
  }

  FIELD_SET(*this, IBAPI::MsgType);
  FIELD_SET(*this, IBAPI::BeginString);
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
  Message() {}
  Message(const std::string& version,
          const std::string& msgType) : header_(version, msgType)
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
