#ifndef IB_API_PROTOCOL_HANDLER_H_
#define IB_API_PROTOCOL_HANDLER_H_

#include <boost/scoped_ptr.hpp>

#include <Shared/EClient.h>
#include <Shared/EWrapper.h>

#include "common.hpp"

namespace ib {
namespace internal {


class ApiSocket {
 public:
  virtual int Send(const char* buf, size_t size) = 0;
  virtual int Receive(char* buf, size_t size) = 0;
  virtual bool IsSocketOK() const = 0;
};


class ApiProtocolHandler : NoCopyAndAssign
{

 public:

  ApiProtocolHandler(EWrapper& ewrapper);
  ~ApiProtocolHandler();

  EClientPtr GetEClient();

  void SetApiSocket(ApiSocket& socket);
  void SetClientId(unsigned int clientId);
  bool IsConnected() const;
  void OnConnect();
  void OnDisconnect();
  bool CheckMessages();
  void OnError(const int id, const int errorCode,
               const std::string& message);
 private:
  class implementation;
  boost::scoped_ptr<implementation> impl_;
};


} // namespace internal
} // namespace ib


#endif //IB_API_PROTOCOL_HANDLER_H_
