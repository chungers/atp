#ifndef IB_API_ZMQ_MESSAGE_H_
#define IB_API_ZMQ_MESSAGE_H_

#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <zmq.hpp>

#include "log_levels.h"
#include "utils.hpp"

#include "ib/internal.hpp"
#include "ib/Message.hpp"



namespace ib {
namespace internal {


using ib::internal::EClientPtr;

class ZmqMessage;

typedef boost::uint64_t MessageId;
typedef boost::uint64_t Timestamp;
typedef boost::optional< boost::shared_ptr<ZmqMessage> > ZmqMessagePtr;

class ZmqMessage : public IBAPI::Message
{

 public:

  ZmqMessage() : timestamp_(now_micros()), messageId_(0)
  {}

  /**
   * Given the buffer read from zmq as the topic / message key,
   * return a message pointer.  This is optional, so in the
   * event that message key does not map to any known message,
   * return an uninitialized ZmqMessagePtr.
   */
  static void createMessage(const std::string& msgKey, ZmqMessagePtr& ptr);

  /**
   * conventions: valid message id must be > 0
   */
  const MessageId& messageId() const
  {
    return messageId_;
  }

  const Timestamp& timestamp() const
  {
    return timestamp_;
  }

  virtual bool validate() = 0;

  virtual bool callApi(EClientPtr eclient) = 0;

  virtual bool receive(zmq::socket_t& socket) = 0;

 protected:
  Timestamp timestamp_;
  MessageId messageId_;
};

} // namespace internal
} // namespace ib

#endif //IB_API_ZMQ_MESSAGE_H_
