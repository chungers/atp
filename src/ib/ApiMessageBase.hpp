#ifndef IB_API_MESSAGE_BASE_H_
#define IB_API_MESSAGE_BASE_H_

#include "log_levels.h"
#include "ib/internal.hpp"
#include "ib/Message.hpp"

#include <zmq.hpp>


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


namespace ib {
namespace internal {


using ib::internal::EClientPtr;


class ApiMessageBase : public IBAPI::Message
{

 public:
  ApiMessageBase(const Message& copy) : IBAPI::Message(copy)
  {
  }

  ApiMessageBase(const IBAPI::MsgType& type, const IBAPI::MsgKey& key) :
      IBAPI::Message(type, key)
  {
  }

 protected:
  ApiMessageBase() : IBAPI::Message()
  {
  }

 public:

  ~ApiMessageBase() {}

  virtual bool callApi(EClientPtr eclient) = 0;
  virtual bool send(zmq::socket_t& detination);
  virtual bool receive(zmq::socket_t& source);
};


class ZmqMessage : public ib::internal::ApiMessageBase
{

 public:
  ZmqMessage() : ApiMessageBase()
  {
  }

  ~ZmqMessage() {}

  virtual bool callApi(EClientPtr eclient)
  { return false; }  // Invalid operation

  virtual bool send(zmq::socket_t& destination)
  { return false; }  // Invalid operation

};

} // namespace internal
} // namespace ib

#endif //IB_API_MESSAGE_BASE_H_
