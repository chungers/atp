#ifndef IB_CONTRACT_SYMBOL_H_
#define IB_CONTRACT_SYMBOL_H_

#include <map>
#include <string>
#include <sstream>

#include "platform/contract_symbol.hpp"


namespace ib {
namespace internal {

using namespace std;
namespace p = proto::ib;

bool symbol_from_contract(const p::Contract& contract, string* output);

/// Map is a map of string to string
template <typename Map>
bool symbol_from_contract(const Map& contract,
                          string* output)
{
  // Build the symbol string here.
  try {
    ostringstream s;
    s << contract.at("symbol") << '.' << contract.at("secType");

    // if (contract.at("secType") != "STK" && contract.at("secType") != "IND") {
    if (contract.at("secType") == "OPT") {
      s << '.'
        << contract.at("expiry")  // assumes the form of YYYYMMDD
        << '.'
        << contract.at("strike")
        << '.'
        << contract.at("right");
    }
    output->assign(s.str());
    return true;

  } catch (...) {
    return false;
  }
}

} // namespace internal
} // namespace ib

#endif //IB_CONTRACT_SYMBOL_H_
