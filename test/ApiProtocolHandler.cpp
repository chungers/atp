
#include <boost/shared_ptr.hpp>

#include <Shared/EClient.h>
#include <Shared/EWrapper.h>

#include "ib/internal.hpp"
#include "ib/ApiProtocolHandler.hpp"
#include "ib/api966/EClientMock.hpp"

#include "common.hpp"
#include "log_levels.h"


namespace ib {
namespace internal {

class ApiProtocolHandler::implementation : NoCopyAndAssign
{
 public:
  implementation(EWrapper* ewrapper,
                 EClientMock* mock) :
      ewrapper_(ewrapper),
      eclientMock_(mock),
      socket_()
  {
  }

  ~implementation()
  {
  }

  EClientPtr GetEClient()
  {
    LOG(ERROR) << "Getting mock";
    return EClientPtr(eclientMock_.get());
  }

  void SetApiSocket(ApiSocket& socket)
  {
    socket_ = &socket;
  }

  void SetClientId(unsigned int clientId)
  {
  }

  bool IsConnected()
  {
    return false;
  }

  void OnConnect()
  {
  }
  void OnDisconnect()
  {
  }

  bool CheckMessages()
  {
    return true;
  }

  void OnError(const int id, const int errorCode,
               const std::string& message)
  {
    ewrapper_->error(id, errorCode, message);
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
  boost::scoped_ptr<EWrapper> ewrapper_;
  boost::scoped_ptr<EClientMock> eclientMock_;
  ApiSocket* socket_;
};



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
