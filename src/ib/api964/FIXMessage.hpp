#ifndef IBAPI_FIX_MESSAGE_H_
#define IBAPI_FIX_MESSAGE_H_

#include <quickfix/FieldMap.h>
#include <quickfix/Fields.h>

#include "utils.hpp"

#include "ib/ZmqMessage.hpp"
#include "zmq/ZmqUtils.hpp"

#include "IBAPIFields.hpp"
#include "IBAPIValues.hpp"



// Macros for mapping FIX fields to some variable / struct memebers:
#define MAP_REQUIRED_FIELD(fix, varName) \
  { fix tmp; get(tmp); varName = tmp; }

#define MAP_OPTIONAL_FIELD_DEFAULT(fix, varName, defaultValue)  \
  { fix tmp; try { get(tmp); varName = tmp; } catch (FIX::FieldNotFound e) { \
      API_MESSAGE_STRATEGY_LOGGER << "Missing optional field: " << #varName; \
      tmp.setString(#defaultValue); varName = tmp; }}

#define MAP_OPTIONAL_FIELD(fix, varName)                       \
  { fix tmp; try { get(tmp); varName = tmp; } catch (FIX::FieldNotFound e) { \
      API_MESSAGE_STRATEGY_LOGGER << "Missing optional field: " << #varName; }}

#define REQUIRED_FIELD(fix, varName)            \
fix varName; get(varName)

#define OPTIONAL_FIELD_DEFAULT(fix, varName, defValue) \
  fix varName; \
  try { get(varName); } \
  catch (FIX::FieldNotFound e) { \
    API_MESSAGE_STRATEGY_LOGGER << "Missing optional field: " << #varName; \
    varName.setString(#defValue); \
  }

#define OPTIONAL_FIELD(fix, varName) \
  fix varName; \
  try { get(varName); } \
  catch (FIX::FieldNotFound e) { \
    API_MESSAGE_STRATEGY_LOGGER << "Missing optional field: " << #varName;   \
  }


using namespace FIX;

namespace ib {
namespace internal {


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

  friend std::ostream& operator<<(std::ostream& os, const Header& m)
  {
    FIX::FieldMap::iterator itr = m.begin();
    os << "HEADER{";
    for (int i = 0; itr != m.end(); ++itr, ++i) {
      if (i > 0) {
        os << ",";
      }
      os << itr->second.getValue();
    }
    os << "}";
    return os;
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

  friend std::ostream& operator<<(std::ostream& os, const Trailer& m)
  {
    FIX::FieldMap::iterator itr = m.begin();
    os << "TRAILER{";
    for (int i = 0; itr != m.end(); ++itr, ++i) {
      if (i > 0) {
        os << ",";
      }
      os << itr->second.getValue();
    }
    os << "}";
    return os;
  }

  FIELD_SET(*this, IBAPI::Ext_SendingTimeMicros);
  FIELD_SET(*this, IBAPI::Ext_OrigSendingTimeMicros);
};


static size_t send_map(zmq::socket_t& socket, const FieldMap& message,
                       bool more = true)
{
  size_t fields = message.totalFields() - 1;
  size_t sent = 0;
  for (FieldMap::iterator f = message.begin();
       f != message.end();
       ++f, --fields) {
    // encode by <field_id>=<field_value> where
    // field_id is a number and field_value is a string representation
    // of the typed value.
    size_t len = f->second.getValue().length();
    const std::string& frame = f->second.getValue();

    // Note that somehow FIX fields have an extra '\0' padded.
    sent += atp::zmq::send_copy(
        socket, frame.data(), len-1, fields > 0 || more);

    API_MESSAGE_BASE_LOGGER << "Sent frame [" << frame << "], len = "
                            << sent << "/" << len;
  }
  return sent;
}


class FIXMessage : public FIX::FieldMap,
                   public ib::internal::ZmqMessage,
                   public ib::internal::ZmqSendable
{
 public:
  FIXMessage() {}
  FIXMessage(const MsgType& msgType,
             const MsgKey& msgKey) :
      header_(msgType, msgKey),
      key_(msgKey)
  {
  }

  FIXMessage(const FIXMessage& copy) :
      FIX::FieldMap(copy),
      ib::internal::ZmqMessage(),
      ib::internal::ZmqSendable(),
      header_(copy.header_),
      trailer_(copy.trailer_)
  {
    FIX::BeginString messageKey;
    getHeader().get(messageKey);
    key_ = messageKey.getString();
  }

  Header& getHeader()
  { return header_; }

  Trailer& getTrailer()
  { return trailer_; }

  virtual const std::string& key() const
  {
    return key_;
  }

  virtual bool validate()
  {
    return true;
  }

  virtual bool callApi(EClientPtr eclient)
  {
    return false;
  }

  virtual size_t send(zmq::socket_t& destination)
  {
    size_t sent = 0;
    // Send the message key first
    FIX::BeginString messageKey;
    getHeader().get(messageKey);
    sent += atp::zmq::send_copy(destination, messageKey.getString(), true);

    // Send the actual message.
    sent += send_map(destination, getHeader(), true);
    sent += send_map(destination, *this, true);
    sent += send_map(destination, getTrailer(), false); // terminating fieldmap
    return sent;
  }

  friend std::ostream& operator<<(std::ostream& os, const FIXMessage& m)
  {
    os << m.header_;
    FIX::FieldMap::iterator itr = m.begin();
    os << "BODY{";
    for (int i = 0; itr != m.end(); ++itr, ++i) {
      if (i > 0) {
        os << ",";
      }
      os << itr->second.getValue();
    }
    os << "}";
    os << m.trailer_;
    return os;
  }

  virtual bool receive(zmq::socket_t& source)
  {
    while (1) {
      std::string buff;
      bool more = (atp::zmq::receive(source, &buff));
      API_MESSAGE_BASE_LOGGER
          << "Received: " << buff
          << ", " << buff.length() << "/" << buff.size();

      parseMessageField(buff, *this);
      if (!more) break;
    }
    return true; // TODO -- some basic check...
  }

 private:

  bool parseMessageField(const std::string& buff, FIXMessage& message)
  {
    // parse the message string
    size_t pos = buff.find('=');
    if (pos != std::string::npos) {
      int code = atoi(buff.substr(0, pos).c_str());
      std::string value = buff.substr(pos+1);
      switch (code) {
        case FIX::FIELD::MsgType:
        case FIX::FIELD::BeginString:
        case FIX::FIELD::SendingTime:
          API_MESSAGE_BASE_LOGGER
              << "H[" << code << "][" << value << "] = "
              << value.size() << "/" << buff.size() << "/" << buff.length();
          message.getHeader().setField(code, value);
          break;
        case FIX::FIELD::Ext_SendingTimeMicros:
        case FIX::FIELD::Ext_OrigSendingTimeMicros:
          API_MESSAGE_BASE_LOGGER
              << "T[" << code << "][" << value << "] = "
              << value.size() << "/" << buff.size() << "/" << buff.length();
          message.getTrailer().setField(code, value);
          break;
        default:
          API_MESSAGE_BASE_LOGGER
              << "B[" << code << "][" << value << "] = "
              << value.size() << "/" << buff.size() << "/" << buff.length();
          message.setField(code, value);
      }
      return true;
    }
    return false;
  }

 protected:
  mutable Header header_;
  mutable Trailer trailer_;

 private:
  std::string key_;
};

} // namespace internal
} // namespace ib

#endif // IBAPI_FIX_MESSAGE_H_
