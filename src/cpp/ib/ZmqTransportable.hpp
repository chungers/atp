#ifndef IB_API_ZMQ_TRANSPORTABLE_H_
#define IB_API_ZMQ_TRANSPORTABLE_H_

#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <zmq.hpp>

#include "ib/Message.hpp"

namespace ib {
namespace internal {


class ZmqSendable
{
 public:
  virtual size_t send(zmq::socket_t& destination,
                      IBAPI::MessageId messageId = 0) = 0;
};


class ZmqReceivable
{
  virtual bool receive(zmq::socket_t& socket) = 0;
};

} // namespace internal
} // namespace ib

#endif //IB_API_ZMQ_TRANSPORTABLE_H_
