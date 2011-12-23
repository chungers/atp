#ifndef IB_INTERNAL_TICKER_MAP_H_
#define IB_INTERNAL_TICKER_MAP_H_

#include <map>
#include <string>
#include <Shared/Contract.h>

namespace ib {
namespace internal {

bool symbol_from_contract(const Contract& contract, std::string* output);

bool symbol_from_contract(const std::map<std::string, std::string>& contract,
                          std::string* output);

bool convert_to_contract(const std::map<std::string, std::string> input,
                         Contract* output);

struct PrintContract
{
  PrintContract(const Contract& c) : c_(c) {}
  const Contract& c_;
  friend std::ostream& operator<<(std::ostream& os, const PrintContract& c)
  {
    os << ","
       << "contract="
       << "conId:" << c.c_.conId
       << ";symbol:" << c.c_.symbol
       << ";secType:" << c.c_.secType
       << ";right:" << c.c_.right
       << ";strike:" << c.c_.strike
       << ";currency:" << c.c_.currency
       << ";multiplier:" << c.c_.multiplier
       << ";expiry:" << c.c_.expiry
       << ";localSymbol:" << c.c_.localSymbol;
    return os;
  }
};

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
