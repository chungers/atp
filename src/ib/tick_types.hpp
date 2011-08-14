#ifndef IB_INTERNAL_TICK_TYPES_H_
#define IB_INTERNAL_TICK_TYPES_H_

#ifndef IB_USE_STD_STRING
#define IB_USE_STD_STRING
#endif

#include <Shared/EWrapper.h>

namespace ib {
namespace internal {

// Macro that converts the enum token into string while stripping the TickType:: prefix.
using std::string;
#define enum_string(name) string(#name).substr(string(#name).find_last_of("::") + 1)

/**
   Printable string names of tick type enums (see EWrapper.h)
*/

const std::string TickTypeNames[] = {
  enum_string(TickType::BID_SIZE),
  enum_string(TickType::BID),
  enum_string(TickType::ASK),
  enum_string(TickType::ASK_SIZE),
  enum_string(TickType::LAST),
  enum_string(TickType::LAST_SIZE),
  enum_string(TickType::HIGH),
  enum_string(TickType::LOW),
  enum_string(TickType::VOLUME),
  enum_string(TickType::CLOSE),
  enum_string(TickType::BID_OPTION_COMPUTATION),
  enum_string(TickType::ASK_OPTION_COMPUTATION),
  enum_string(TickType::LAST_OPTION_COMPUTATION),
  enum_string(TickType::MODEL_OPTION),
  enum_string(TickType::OPEN),
  enum_string(TickType::LOW_13_WEEK),
  enum_string(TickType::HIGH_13_WEEK),
  enum_string(TickType::LOW_26_WEEK),
  enum_string(TickType::HIGH_26_WEEK),
  enum_string(TickType::LOW_52_WEEK),
  enum_string(TickType::HIGH_52_WEEK),
  enum_string(TickType::AVG_VOLUME),
  enum_string(TickType::OPEN_INTEREST),
  enum_string(TickType::OPTION_HISTORICAL_VOL),
  enum_string(TickType::OPTION_IMPLIED_VOL),
  enum_string(TickType::OPTION_BID_EXCH),
  enum_string(TickType::OPTION_ASK_EXCH),
  enum_string(TickType::OPTION_CALL_OPEN_INTEREST),
  enum_string(TickType::OPTION_PUT_OPEN_INTEREST),
  enum_string(TickType::OPTION_CALL_VOLUME),
  enum_string(TickType::OPTION_PUT_VOLUME),
  enum_string(TickType::INDEX_FUTURE_PREMIUM),
  enum_string(TickType::BID_EXCH),
  enum_string(TickType::ASK_EXCH),
  enum_string(TickType::AUCTION_VOLUME),
  enum_string(TickType::AUCTION_PRICE),
  enum_string(TickType::AUCTION_IMBALANCE),
  enum_string(TickType::MARK_PRICE),
  enum_string(TickType::BID_EFP_COMPUTATION),
  enum_string(TickType::ASK_EFP_COMPUTATION),
  enum_string(TickType::LAST_EFP_COMPUTATION),
  enum_string(TickType::OPEN_EFP_COMPUTATION),
  enum_string(TickType::HIGH_EFP_COMPUTATION),
  enum_string(TickType::LOW_EFP_COMPUTATION),
  enum_string(TickType::CLOSE_EFP_COMPUTATION),
  enum_string(TickType::LAST_TIMESTAMP),
  enum_string(TickType::SHORTABLE),
  enum_string(TickType::FUNDAMENTAL_RATIOS),
  enum_string(TickType::RT_VOLUME),
  enum_string(TickType::HALTED),
  enum_string(TickType::BID_YIELD),
  enum_string(TickType::ASK_YIELD),
  enum_string(TickType::LAST_YIELD),
  enum_string(TickType::CUST_OPTION_COMPUTATION),
  enum_string(TickType::NOT_SET)
};

/**
   For requesting GenericTick through the IB API.
   A comma-separated string is constructed from the numeric values of these
   enums to request for specific market data or tick by tick computations.
   
   See http://goo.gl/LKfOD
*/
enum GenericTickType {

  // Optiaon Volume (currently for stocks)
    OPTION_VOLUME = 100, // 29, 30

    // Option Open Interest (currently for stocks)
    OPTION_OPEN_INTEREST = 101, // 27, 28

    // Historical Volatility (currently for stocks)
    HISTORICAL_VOLATILITY = 104, // 23

    // Option Implied Volatility (currently for stocks)
    OPTION_IMPLIED_VOLATILITY = 106, // 24

    // Index Future Premium
    INDEX_FUTURE_PREMIUM = 162, // 31

    MISCELLANEOUS_STATS = 165, // 15, 16, 17, 18, 19, 20, 21

    // Mark Price (used in TWS P&L computations) 
    MARK_PRICE = 221, // 37

    // Auction values (volume, price, and imbalance)
    AUCTION_VALUES = 225, // 34, 35, 36

    // RTVolume
    RTVOLUME = 233, // 48

    SHORTABLE = 236, // 46

    INVENTORY = 256, // ?
  
    FUNDAMENTAL_RATIOS = 258, // 47

    REALTIME_HISTORICAL_VOLATILITY = 411 // 58
  };


} // internal
} // ib

#endif //IB_INTERANL_TICK_TYPES_H_
