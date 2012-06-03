#ifndef IB_INTERNAL_H_
#define IB_INTERNAL_H_

#include <zmq.hpp>
#include <boost/shared_ptr.hpp>

#include <Shared/EClientSocketBase.h>
#include <Shared/EClient.h>

#include "ib/ticker_id.hpp"



namespace ib {
namespace internal {


class EWrapperEventCollector
{
 public:
  ~EWrapperEventCollector() {}

  /// Returns the ZMQ socket that will be written to.
  virtual zmq::socket_t* getOutboundSocket(int channel = 0) = 0;

  // TODO: add a << operator
};

typedef boost::shared_ptr<EClient> EClientPtr;


} // internal
} // ib


#endif //IB_INTERNAL_H_
