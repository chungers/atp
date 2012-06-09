
#include <iostream>
#include <string>

#include <boost/date_time/posix_time/posix_time.hpp>

#include "proto/common.hpp"
#include "proto/historian.hpp"
#include "historian/time_utils.hpp"


// This file contains the instantiations of templates.

namespace proto {
namespace historian {

using proto::ib::MarketData;
using proto::ib::MarketDepth;
using proto::historian::IndexedValue;
using proto::historian::SessionLog;
using proto::historian::Record;

template const Record wrap<MarketData>(const MarketData& v);
template const Record wrap<MarketDepth>(const MarketDepth& v);
template const Record wrap<SessionLog>(const SessionLog& v);
template const Record wrap<IndexedValue>(const IndexedValue& v);

} // historian
} // proto

//} // historian


