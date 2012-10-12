
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

} // namespace internal
} // namespace ib

