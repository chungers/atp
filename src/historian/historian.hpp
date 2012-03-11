#ifndef HISTORIAN_H_
#define HISTORIAN_H_

#include "historian/constants.hpp"
#include "historian/time_utils.hpp"
#include "historian/Db.hpp"

namespace historian {

typedef proto::common::Value Value;
typedef proto::ib::MarketData MarketData;
typedef proto::ib::MarketDepth MarketDepth;
typedef proto::historian::SessionLog SessionLog;

} // historian
#endif //HISTORIAN_H_
