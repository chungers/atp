#ifndef IB_API_ECLIENT_H_
#define IB_API_ECLIENT_H_

#include "common.hpp"
#include "ApiImpl.hpp"

namespace ib {
namespace internal {


/**
 * Base class for IB clients
 */
class IBClient : public LoggingEClientSocket, NoCopyAndAssign
{
 public:
  IBClient(EWrapper* ewrapper) : LoggingEClientSocket(ewrapper)
  {
  }

  ~IBClient() {}
};

} // namespace internal
} // namespace ib

#endif //IB_API_ECLIENT_H_
