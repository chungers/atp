#ifndef ATP_PLATFORM_CONTRACT_SYMBOL_H_
#define ATP_PLATFORM_CONTRACT_SYMBOL_H_

#include <string>

#include "proto/ib.pb.h"


namespace atp {
namespace platform {


namespace p = proto::ib;

bool format_option_contract_key(
    const std::string& symbol,
    const double& strike,
    const p::Contract::Right& right,
    const int& year,
    const int& month,
    const int& day,
    std::string* output);

bool symbol_from_contract(const p::Contract& contract, std::string* output);

bool parse_date(const std::string& date, int* year, int* month, int* day);

bool format_expiry(const int& year, const int& month, const int& day,
                   std::string* output);

// symbol = AAPL.STK or AAPL.OPT.20121216.650.C
bool parse_encoded_symbol(const std::string& key,
                          std::string* symbol,
                          p::Contract::Type* sec_type,
                          double* strike,
                          p::Contract::Right* right,
                          int* year,
                          int* month,
                          int* day);

} // platform
} // atp

#endif //ATP_PLATFORM_CONTRACT_SYMBOL_H_
