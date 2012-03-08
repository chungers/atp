#ifndef HISTORIAN_H_
#define HISTORIAN_H_

#include "historian/time_utils.hpp"
#include "historian/Db.hpp"

namespace historian {

const static std::string ENTITY_SESSION_LOG("sessionlog");
const static std::string ENTITY_IB_MARKET_DATA("mkt");
const static std::string ENTITY_IB_MARKET_DEPTH("depth");
const static std::string INDEX_IB_MARKET_DATA_BY_EVENT("x/event-value");

}


#endif //HISTORIAN_H_
