#ifndef IB_API_MESSAGE_BASE_H_
#define IB_API_MESSAGE_BASE_H_

#include "log_levels.h"
#include "ib/ib_common.hpp"
#include "ib/internal.hpp"
#include "ib/Message.hpp"


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


namespace IBAPI {

using ib::internal::EClientPtr;


class ApiMessageBase : public IBAPI::Message
{

 public:
  ApiMessageBase(const Message& copy) : IBAPI::Message(copy)
  {
  }

  ApiMessageBase(const std::string& version, const std::string& name) :
      IBAPI::Message(version, name)
  {
  }

  ~ApiMessageBase() {}

  virtual bool callApi(EClientPtr eclient) = 0;
};

}

#endif //IB_API_MESSAGE_BASE_H_
