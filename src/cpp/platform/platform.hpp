#ifndef ATP_PLATFORM_PLATFORM_H_
#define ATP_PLATFORM_PLATFORM_H_

#include <string>
#include "platform/types.hpp"


namespace atp {
namespace platform {

const static std::string DATA_END("__DATA_END__");

// For sending and receiving control messages -- first frame/ header
const static std::string CONTROL_MESSAGE("__CONTROL__");

// For sending and receiving timer messages
const static std::string TIMER_MESSAGE("__TIMER__");

}
}
#endif // ATP_PLATFORM_PLATFORM_H_
