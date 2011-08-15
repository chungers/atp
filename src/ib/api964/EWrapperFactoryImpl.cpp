

#include "ib/EWrapperFactory.hpp"
#include "ApiImpl.hpp"
#include "EventDispatcher.hpp"


namespace IBAPI {

/// Implementation of EWrapperFactory
EWrapperFactory::EWrapperFactory() {
  LOG(INFO) << "EWrapperFactory start." << std::endl;
}

EWrapperFactory::~EWrapperFactory() {
  LOG(INFO) << "EWrapperFactory done." << std::endl;
}

/// Implements EWrapperFactory
EWrapper* EWrapperFactory::getImpl() {
  return new ib::internal::TestEWrapper();
}

} // IBAPI


