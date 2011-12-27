
#include <map>
#include <string>
#include <sstream>

#include <glog/logging.h>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>

#include "log_levels.h"
#include "utils.hpp"
#include "ib/TickerMap.hpp"
#include "ib/ticker_id.hpp"

namespace ib {
namespace internal {

typedef boost::shared_ptr< Contract > ContractPtr;

static std::map< long, std::string > TICKER_ID_SYMBOL_MAP;
static std::map< std::string, long > SYMBOL_TICKER_ID_MAP;
static std::map< long, ContractPtr > TICKER_ID_CONTRACT_MAP;

static boost::mutex __ticker_map_mutex;

ContractPtr clone_contract(const Contract& contract)
{
  ContractPtr c(new Contract());
  c->conId = contract.conId;
  c->symbol = contract.symbol;
  c->localSymbol = contract.localSymbol;
  c->secIdType = contract.secIdType;
  c->secId = contract.secId;
  c->secType = contract.secType;
  c->strike = contract.strike;
  c->right = contract.right;
  c->exchange = contract.exchange;
  c->primaryExchange = contract.primaryExchange;
  c->includeExpired = contract.includeExpired;
  c->expiry = contract.expiry;
  c->multiplier = contract.multiplier;
  c->currency = contract.currency;
  return c;
}

bool symbol_from_contract(const std::map<std::string, std::string>& nv,
                          std::string* output)
{
  // Build the symbol string here.
  try {
    std::ostringstream s;
    s << nv.at("symbol") << '.' << nv.at("secType");

    if (nv.at("secType") != "STK" || nv.at("secType") != "IND") {
      s << '.'
        << nv.at("strike")
        << '.'
        << nv.at("right")
        << '.'
        << nv.at("expiry");
    }
    *output = s.str();
    return true;

  } catch (...) {
    return false;
  }
}

bool symbol_from_contract(const Contract& contract, std::string* output)
{
  std::ostringstream ss;
  ss << contract.symbol << '.' << contract.secType;

  if (contract.secType != "STK" || contract.secType != "IND") {
    ss << '.'
       << contract.strike
       << '.'
       << contract.right
       << '.'
       << contract.expiry;
  }
  *output = ss.str();
  return true;
}

bool convert_to_contract(const std::map<std::string, std::string> input,
                         Contract* output)
{
  try {
    ::from_string(input.at("conId"), &(output->conId));
    output->symbol = input.at("symbol");
    output->secType = input.at("secType");
    output->currency = input.at("currency");
    output->localSymbol = input.at("localSymbol");

    if (output->secType != "STK") {
      output->right = input.at("right");
      ::from_string(input.at("strike"), &(output->strike));
      output->multiplier = input.at("multiplier");
      output->expiry = input.at("expiry");
    }

  } catch (std::out_of_range e) {
    TICKERMAP_ERROR << "Exception converting to contract: " << e.what();
    return false;
  }
  return true;
}

long TickerMap::registerContract(const Contract& contract)
{
  boost::unique_lock<boost::mutex> lock(__ticker_map_mutex);

  // Use contract.conId as the key
  long tickerId = contract.conId;

  if (tickerId == 0) {
      tickerId = TICKER_ID_CONTRACT_MAP.size() + 1;
  }

  if (TICKER_ID_SYMBOL_MAP.find(tickerId) == TICKER_ID_SYMBOL_MAP.end()) {

    ContractPtr clone = clone_contract(contract);
    clone->conId = tickerId;
    std::string symbol;
    if (symbol_from_contract(contract, &symbol)) {
      TICKER_ID_SYMBOL_MAP[tickerId] = symbol;
      SYMBOL_TICKER_ID_MAP[symbol] = tickerId;
      TICKER_ID_CONTRACT_MAP[tickerId] = clone;
    } else {
      // Error
      tickerId = -1;
    }
  } else {
    // There's an entry.  Verify that the symbol is the same
    std::string check;
    if (symbol_from_contract(contract, &check)) {
      if (TICKER_ID_SYMBOL_MAP[tickerId] != check) {
        tickerId = -1; // Error
      }
    } else {
      tickerId = -1; // Error
    }
  }
  return tickerId;
}

bool TickerMap::getSubscriptionKeyFromId(long tickerId, std::string* output)
{
  if (TICKER_ID_SYMBOL_MAP.find(tickerId) == TICKER_ID_SYMBOL_MAP.end()) {
    // Use conversion
    ib::internal::SymbolFromTickerId(tickerId, output);
    return false; // Since we don't have any mappings.
  } else {
    *output = TICKER_ID_SYMBOL_MAP[tickerId];
    return true;
  }
}

bool TickerMap::getTickerIdFromSubscriptionKey(const std::string& key, long* id)
{
  if (SYMBOL_TICKER_ID_MAP.find(key) == SYMBOL_TICKER_ID_MAP.end()) {
    // Use conversion
    *id = ib::internal::SymbolToTickerId(key);
    return false; // Since we don't have any mappings.
  } else {
    *id = SYMBOL_TICKER_ID_MAP[key];
    return true;
  }
}
} // namespace internal
} // namespace ib

