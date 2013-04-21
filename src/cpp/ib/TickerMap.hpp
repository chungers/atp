#ifndef IB_INTERNAL_TICKER_MAP_H_
#define IB_INTERNAL_TICKER_MAP_H_

#include <map>
#include <string>
#include <Shared/Contract.h>

#include "ib/contract_symbol.hpp"

namespace ib {
namespace internal {

bool symbol_from_contract(const Contract& contract, std::string* output);

bool convert_to_contract(const std::map<std::string, std::string> input,
                         Contract* output);

/**
 * Interface for mapping ticker ids to contracts and symbols.
 */
class TickerMap
{
 public:

  /// Registers the contract and assigns a unique id for use with reqMktData.
  static long registerContract(const Contract& contract);

  /// Given the id, get a contract symbol.
  static bool getSubscriptionKeyFromId(long tickerId, std::string* output);

  /// Given the subscription key, get the tickerId.
  static bool getTickerIdFromSubscriptionKey(const std::string& key, long* id);
};

} // namespace internal
} // namespace ib

#endif //IB_INTERNAL_TICKER_MAP_H_
