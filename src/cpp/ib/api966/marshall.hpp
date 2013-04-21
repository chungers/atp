#ifndef IBAPI_MARSHALL_H_
#define IBAPI_MARSHALL_H_

#include <Shared/Contract.h>
#include <Shared/Order.h>
#include "log_levels.h"
#include "ib/internal.hpp"
#include "ib/TickerMap.hpp"
#include "proto/ib.pb.h"

namespace p = proto::ib;


bool ParseDate(const std::string& date, int* year, int* month, int* day);


//////////////// CONTRACT /////////////////

bool operator<<(Contract& c, const p::Contract& p);
bool operator<<(p::Contract& p, const Contract& c);

inline bool operator>>(const p::Contract& p, Contract& c)
{ return c << p; }

inline bool operator>>(const Contract& c, p::Contract& p)
{ return p << c; }


//////////////// CONTRACT DETAILS /////////////////

bool operator<<(ContractDetails& c, const p::ContractDetails& p);
bool operator<<(p::ContractDetails& p, const ContractDetails& c);

inline bool operator>>(const p::ContractDetails& p, ContractDetails& c)
{ return c << p; }

inline bool operator>>(const ContractDetails& c, p::ContractDetails& p)
{ return p << c; }


//////////////// ORDER /////////////////

// Order base

bool operator<<(Order& o, const p::Order& p);
bool operator<<(p::Order& p, const Order& o);

inline bool operator>>(const p::Order& p, Order& o)
{ return o << p; }

inline bool operator>>(const Order& o, p::Order& p)
{ return p << o; }

// Specific order types:

// MarketOrder --------------

bool operator<<(Order& o, const p::MarketOrder& p);
bool operator<<(p::MarketOrder& p, const Order& o);

inline bool operator>>(const p::MarketOrder& p, Order& o) { return o << p; }
inline bool operator>>(const Order& o, p::MarketOrder& p) { return p << o; }


// LimitOrder --------------

bool operator<<(Order& o, const p::LimitOrder& p);
bool operator<<(p::LimitOrder& p, const Order& o);

inline bool operator>>(const p::LimitOrder& p, Order& o) { return o << p; }
inline bool operator>>(const Order& o, p::LimitOrder& p) { return p << o; }

// StopLimitOrder --------------

bool operator<<(Order& o, const p::StopLimitOrder& p);
bool operator<<(p::StopLimitOrder& p, const Order& o);

inline bool operator>>(const p::StopLimitOrder& p, Order& o) { return o << p; }
inline bool operator>>(const Order& o, p::StopLimitOrder& p) { return p << o; }



#endif // IBAPI_MARSHALL_H_
