#ifndef IB_API_ZMQ_MESSAGE_H_
#define IB_API_ZMQ_MESSAGE_H_

#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <zmq.hpp>

#include "log_levels.h"
#include "ib/internal.hpp"
#include "ib/Message.hpp"



namespace ib {
namespace internal {


using ib::internal::EClientPtr;

class ZmqMessage;

typedef boost::optional< boost::shared_ptr<ZmqMessage> > ZmqMessagePtr;

class ZmqMessage : public IBAPI::Message
{

 public:

  /**
   * Given the buffer read from zmq as the topic / message key,
   * return a message pointer.  This is optional, so in the
   * event that message key does not map to any known message,
   * return an uninitialized ZmqMessagePtr.
   */
  static void createMessage(const std::string& msgKey, ZmqMessagePtr& ptr);


  virtual const std::string& key() const = 0;

  virtual bool validate() = 0;

  virtual bool callApi(EClientPtr eclient) = 0;

  virtual bool receive(zmq::socket_t& socket) = 0;
};

class ZmqSendable
{
 public:

  virtual size_t send(zmq::socket_t& destination) = 0;
};

} // namespace internal
} // namespace ib

#endif //IB_API_ZMQ_MESSAGE_H_
