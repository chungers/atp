#ifndef IB_CONTRACT_SYMBOL_H_
#define IB_CONTRACT_SYMBOL_H_

#include <map>
#include <string>


namespace ib {
namespace internal {

bool symbol_from_contract(const std::map<std::string, std::string>& contract,
                          std::string* output);

} // namespace internal
} // namespace ib

#endif //IB_CONTRACT_SYMBOL_H_
