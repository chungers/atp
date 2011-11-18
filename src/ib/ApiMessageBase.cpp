
#include "log_levels.h"
#include "ib/ApiMessageBase.hpp"
#include "ib/internal.hpp"
#include "zmq/ZmqUtils.hpp"


namespace ib {
namespace internal {

using IBAPI::Message;
using FIX::FieldMap;
using ib::internal::ApiMessageBase;


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
    const std::string& frame = f->second.getValue();
    sent += atp::zmq::send_copy(socket, frame, fields > 0 || more);

    API_MESSAGE_BASE_LOGGER << "Sent frame [" << frame << "], bytes = "
                            << sent;
  }
  return sent;
}

bool ApiMessageBase::send(zmq::socket_t& destination)
{
  size_t sent = 0;
  sent += send_map(destination, getHeader(), true);
  sent += send_map(destination, *this, true);
  sent += send_map(destination, getTrailer(), false); // terminating fieldmap
  return sent > 0;
}

using IBAPI::Message;
static bool parseMessageField(const std::string& buff, Message& message)
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

bool ApiMessageBase::receive(zmq::socket_t& source)
{
  std::string buff;
  while (1) {
    bool more = (atp::zmq::receive(source, &buff));
    parseMessageField(buff, *this);
    if (!more) break;
  }
  return true; // TODO -- some basic check...
}


} // namespace internal
} // namespace ib
