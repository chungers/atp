#ifndef IB_API_MESSAGE_BASE_H_
#define IB_API_MESSAGE_BASE_H_


#include "ib/ib_common.hpp"
#include "ib/internal.hpp"
#include "ib/Message.hpp"

namespace IBAPI {

using ib::internal::EClientPtr;


class ApiMessageBase : public IBAPI::Message
{

 public:
  ApiMessageBase(const std::string& version, const std::string& name) :
      IBAPI::Message(version, name)
  {
  }

  ~ApiMessageBase() {}

  virtual bool callApi(EClientPtr eclient) = 0;
};

}

#endif //IB_API_MESSAGE_BASE_H_
