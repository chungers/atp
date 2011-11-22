#ifndef IB_INTERNAL_H_
#define IB_INTERNAL_H_

#include <boost/shared_ptr.hpp>

#include "ib/ticker_id.hpp"

// All versioned implementation must provide this header
// file which defines the class EClient as a subclass of
// some adapter class that implements IB's EClient interface.
#include "EClient.hpp"


namespace ib {
namespace internal {


typedef boost::shared_ptr<IBClient> EClientPtr;


} // internal
} // ib


#endif //IB_INTERNAL_H_
