#ifndef IBAPI_API_EVENT_DISPATCHER_H_
#define IBAPI_API_EVENT_DISPATCHER_H_


#include <boost/shared_ptr.hpp>
#include <zmq.hpp>

#include <Shared/EWrapper.h>

#include "ib/Application.hpp"
#include "ib/SessionID.hpp"

namespace IBAPI {

class OutboundChannels
{
 public:

  virtual zmq::socket_t* getOutboundSocket(unsigned int channel = 0) = 0;

};

class ApiEventDispatcher
{
 public:

  ApiEventDispatcher(IBAPI::Application& app,
                     const IBAPI::SessionID& sessionId) :
      app_(app),
      sessionId_(sessionId),
      outboundChannels_(NULL)
  {
  }

  ~ApiEventDispatcher()
  {
  }


  void SetOutboundSockets(OutboundChannels& channels)
  {
    outboundChannels_ = &channels;
  }

  virtual EWrapper* GetEWrapper() = 0;


 protected:

  const IBAPI::SessionID& SessionId() const
  {
    return sessionId_;
  }

  IBAPI::Application& Application() const
  {
    return app_;
  }

  zmq::socket_t* getOutboundSocket(unsigned int channel = 0)
  {
    if (outboundChannels_ != NULL) {
      return outboundChannels_->getOutboundSocket(channel);
    }
    return NULL;
  }

  IBAPI::Application& app_;
  const IBAPI::SessionID& sessionId_;
  OutboundChannels* outboundChannels_;
};


} // IBAPI


#endif //IBAPI_API_EVENT_DISPATCHER_H_
