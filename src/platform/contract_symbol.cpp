
#include <string>
#include <sstream>
#include <vector>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

#include "platform/contract_symbol.hpp"


namespace atp {
namespace platform {

namespace p = proto::ib;



bool parse_date(const std::string& date, int* year, int* month, int* day)
{
  if (date.length() != 8) return false;
  *year = boost::lexical_cast<int>(date.substr(0,4));
  *month = boost::lexical_cast<int>(date.substr(4,2));
  *day = boost::lexical_cast<int>(date.substr(6,2));
  return true;
}

bool format_expiry(const int& year, const int& month, const int& day,
                   std::string* output)
{
  try {
    std::ostringstream s1;
    std::string fmt = (month > 9) ? "%4d%2d" : "%4d0%1d";
    std::string fmt2 = (day > 9) ? "%2d" : "0%1d";
    s1 << boost::format(fmt) % year % month << boost::format(fmt2) % day;
    output->assign(s1.str());
  return true;
  } catch (...) {
    return false;
  }
}

bool format_option_contract_key(
    const std::string& symbol,
    const double& strike,
    const p::Contract::Right& right,
    const int& year,
    const int& month,
    const int& day,
    std::string* output)
{
  try {
    std::ostringstream ss;
    ss << symbol << ".OPT";

    std::string expiry;
    if (!format_expiry(year, month, day, &expiry)) {
      return false;
    }

    ss << '.'
       << expiry
       << '.'
       << strike
       << '.';
    switch (right) {
      case (p::Contract::CALL) :
        ss << "C"; break;
      case (p::Contract::PUT) :
        ss << "P"; break;
      default:
        return false;
    }

    output->assign(ss.str());

    return true;
  } catch (...) {
    return false;
  }
}

bool symbol_from_contract(const p::Contract& contract, std::string* output)
{
  try {
    std::ostringstream symbol;
    switch (contract.type()) {
      case p::Contract::STOCK:
        symbol << contract.symbol() << ".STK";
        output->assign(symbol.str());
        return true;

      case p::Contract::INDEX:
        symbol << contract.symbol() << ".IND";
        output->assign(symbol.str());
        return true;

      case p::Contract::OPTION:
        return format_option_contract_key(
            contract.symbol(),
            contract.strike().amount(),
            contract.right(),
            contract.expiry().year(),
            contract.expiry().month(),
            contract.expiry().day(),
            output);
      default :
        return false;
    }
  } catch (...) {
    return false;
  }
}

// symbol = AAPL.STK or AAPL.OPT.20121216.650.C
bool parse_encoded_symbol(const std::string& key,
                          std::string* symbol,
                          p::Contract::Type* sec_type,
                          double* strike,
                          p::Contract::Right* right,
                          int* year,
                          int* month,
                          int* day)
{
  using namespace std;
  vector<string> parts;
  boost::split(parts, key, boost::is_any_of("."));

  if (parts.size() < 2) return false;

  *symbol = parts[0];
  if (parts[1] == "STK") {
    *sec_type = p::Contract::STOCK;
    return true;
  }

  if (parts[1] == "IND") {
    *sec_type = p::Contract::INDEX;
    return true;
  }

  if (parts[1] == "OPT" && parts.size() == 5) {
    *sec_type = p::Contract::OPTION;
    *strike = boost::lexical_cast<double>(parts[2]);

    if (!parse_date(parts[4], year, month, day)) return false;

    if (parts[4] == "P") *right = p::Contract::PUT;
    if (parts[4] == "C") *right = p::Contract::CALL;

    return true;

  } else {
    return false;
  }

}

} // platform
} // atp
