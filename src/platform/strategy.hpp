#ifndef ATP_PLATFORM_SIGNAL_H_
#define ATP_PLATFORM_SIGNAL_H_

#include <boost/scoped_ptr.hpp>

#include "proto/platform.pb.h"


using boost::scoped_ptr;
using proto::platform::Id;
using proto::platform::Signal;
using proto::platform::Strategy;



namespace atp {
namespace platform {

class strategy
{
 public:
  strategy(const Strategy& strategyProto);

 private:

  class implementation;
  scoped_ptr<implementation> impl_;


};

} // platform
} // atp


#endif // ATP_PLATFORM_SIGNAL_H_
