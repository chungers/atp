#ifndef PROTO_OSTREAM_H_
#define PROTO_OSTREAM_H_

#include <iostream>

#include "proto/common.pb.h"
#include "proto/historian.pb.h"
#include "proto/ib.pb.h"



using proto::common::Date;
using proto::common::DateTime;
using proto::common::Time;
using proto::common::Money;
using proto::common::Value;
using proto::ib::MarketData;
using proto::ib::MarketDepth;
using proto::ib::Order;
using proto::ib::OrderStatus;
using proto::historian::IndexedValue;
using proto::historian::SessionLog;
using proto::historian::Record;
using namespace proto::common;
using namespace proto::historian;

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
