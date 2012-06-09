#ifndef PROTO_OSTREAM_H_
#define PROTO_OSTREAM_H_

#include <iostream>

#include "proto/common.pb.h"
#include "proto/historian.pb.h"
#include "proto/ib.pb.h"

using namespace proto::common;
using namespace proto::historian;
using namespace proto::ib;

std::ostream& operator<<(std::ostream& out, const Date& v);

std::ostream& operator<<(std::ostream& out, const Time& v);

std::ostream& operator<<(std::ostream& out, const DateTime& v);

std::ostream& operator<<(std::ostream& out, const Money& v);

std::ostream& operator<<(std::ostream& out, const Value& v);

std::ostream& operator<<(std::ostream& out, const MarketData& v);

std::ostream& operator<<(std::ostream& out, const MarketDepth& v);

std::ostream& operator<<(std::ostream& out, const IndexedValue& iv);

std::ostream& operator<<(std::ostream& out, const SessionLog& log);

std::ostream& operator<<(std::ostream& out, const QueryByRange& q);

std::ostream& operator<<(std::ostream& out, const QueryBySymbol& q);

std::ostream& operator<<(std::ostream& os, const OrderStatus& o);


#endif //PROTO_OSTREAM_H_
