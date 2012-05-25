#ifndef IBAPI_MARSHALL_H_
#define IBAPI_MARSHALL_H_

#include <Shared/Contract.h>
#include "log_levels.h"
#include "ib/internal.hpp"
#include "ib/TickerMap.hpp"
#include "proto/ib.pb.h"


bool ParseDate(const std::string& date, int* year, int* month, int* day);

bool operator<<(Contract& c, const proto::ib::Contract& p);
bool operator<<(proto::ib::Contract& p, const Contract& c);

inline bool operator>>(const proto::ib::Contract& p, Contract& c)
{ return c << p; }

inline bool operator>>(const Contract& c, proto::ib::Contract& p)
{ return p << c; }

std::ostream& operator<<(std::ostream& os, const Contract& c);

#endif // IBAPI_MARSHALL_H_
