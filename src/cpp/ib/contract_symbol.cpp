
#include <map>
#include <string>
#include <boost/unordered_map.hpp>

#include "ib/contract_symbol.hpp"


namespace ib {
namespace internal {

using namespace std;
using boost::unordered_map;

/// template function instantiations
bool symbol_from_contract(const map<string, string>& contract, string* output);

bool symbol_from_contract(const unordered_map<string, string>& contract,
                          string* output);

namespace p = proto::ib;
bool symbol_from_contract(const p::Contract& contract, string* output)
{
  return atp::platform::symbol_from_contract(contract, output);
}

} // namespace internal
} // namespace ib

