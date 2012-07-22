
#include <boost/shared_ptr.hpp>

#include <Shared/EClientSocketBaseImpl.h>

#include "ib/internal.hpp"
#include "ib/ApiProtocolHandler.hpp"

#include "common.hpp"
#include "log_levels.h"
#include "ApiImpl.hpp"

namespace ib {
namespace internal {


class ApiProtocolHandler::implementation :
      public LoggingEClientSocket, NoCopyAndAssign
{
 public:
  implementation(EWrapper& ewrapper) : LoggingEClientSocket(&ewrapper),
                                       socket_()
  {
  }

  ~implementation()
  {
  }

  EClientPtr GetEClient()
  {
    return EClientPtr(this);
  }

  void SetApiSocket(ApiSocket& socket)
  {
    socket_ = &socket;
  }

  void SetClientId(unsigned int clientId)
  {
    setClientId(clientId);
  }

  bool IsConnected()
  {
    return isConnected();
  }

  void OnConnect()
  {
    onConnectBase();
  }
  void OnDisconnect()
  {
    eDisconnectBase();
  }

  bool CheckMessages()
  {
    return checkMessages();
  }

  void OnError(const int id, const int errorCode,
               const std::string& message)
  {
    getWrapper()->error(id, errorCode, message);
  }

  //////////////////////////////////////////////////////
 public:

  virtual bool eConnect(const char *host, unsigned int port, int clientId=0)
  {
    return false;
  }

  virtual void eDisconnect()
  {
    return;
  }

 private:

  virtual bool isSocketOK() const
  {
    return socket_->IsSocketOK();
  }

  virtual int send(const char* buf, size_t sz)
  {
    return socket_->Send(buf, sz);
  }

  virtual int receive(char* buf, size_t sz)
  {
    return socket_->Receive(buf, sz);
  }

 private:
  ApiSocket* socket_;
  //boost::shared_ptr<ApiSocket> socket_;
};


ApiProtocolHandler::ApiProtocolHandler(EWrapper& ewrapper) :
    impl_(new implementation(ewrapper))
{
  LOG(INFO) << "Using v966 ApiProtocolHandler.";
}

ApiProtocolHandler::~ApiProtocolHandler()
{
}

EClientPtr ApiProtocolHandler::GetEClient()
{
  return impl_->GetEClient();
}

void ApiProtocolHandler::SetApiSocket(ApiSocket& socket)
{
  impl_->SetApiSocket(socket);
}

void ApiProtocolHandler::SetClientId(unsigned int clientId)
{
  impl_->SetClientId(clientId);
}

bool ApiProtocolHandler::IsConnected() const
{
  return impl_->IsConnected();
}

void ApiProtocolHandler::OnConnect()
{
  impl_->OnConnect();
}

void ApiProtocolHandler::OnDisconnect()
{
  impl_->OnDisconnect();
}

bool ApiProtocolHandler::CheckMessages()
{
  return impl_->CheckMessages();
}

void ApiProtocolHandler::OnError(const int id, const int errorCode,
                                 const std::string& message)
{
  impl_->OnError(id, errorCode, message);
}


} // internal
} // ib
