#ifndef TEST_VERSION_SPECIFIC_H_
#define TEST_VERSION_SPECIFIC_H_

#include "ib/ApiMessageBase.hpp"

// This is the version-specific header where the implementations
// are to be provided by versioned code at build time.

namespace ib {
namespace testing {

class VersionSpecific
{

 private:
  VersionSpecific() {}
  ~VersionSpecific() {}

 public:

  static ib::internal::ApiMessageBase* getMarketDataRequestForTest();

};

} // namespace testing
} // namespace ib

#endif //TEST_VERSION_SPECIFIC_H_
