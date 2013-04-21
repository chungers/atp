#ifndef IB_INTERNAL_H_
#define IB_INTERNAL_H_

#include <zmq.hpp>
#include <boost/shared_ptr.hpp>

#include <Shared/EClientSocketBase.h>
#include <Shared/EClient.h>

#include "ib/ticker_id.hpp"



namespace ib {
namespace internal {


typedef boost::shared_ptr<EClient> EClientPtr;


} // internal
} // ib


#endif //IB_INTERNAL_H_
