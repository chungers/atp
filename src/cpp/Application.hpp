#ifndef ATP_APPLICATION_H_
#define ATP_APPLICATION_H_

#include <string>

#include "common/Factory.hpp"

using std::string;
using atp::common::Factory;


namespace atp {

template <class MessageType>
class Application {

 public :

  virtual ~Application() {};

  virtual Factory<MessageType, string> GetMessageFactory() = 0;

};

} // namespace IBAPI

#endif // ATP_APPLICATION_H_
