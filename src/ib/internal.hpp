#ifndef IB_INTERNAL_H_
#define IB_INTERNAL_H_

#include <boost/shared_ptr.hpp>
#include <Shared/EClient.h>


namespace ib {
namespace internal {

typedef boost::shared_ptr<EClient> EClientPtr;


} // internal
} // ib


#endif //IB_INTERNAL_H_
