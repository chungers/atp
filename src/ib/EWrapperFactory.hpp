#ifndef IBAPI_EWRAPPER_FACTORY_FACTORY_H_
#define IBAPI_EWRAPPER_FACTORY_FACTORY_H_

#include <Shared/EWrapper.h>

namespace IBAPI {


class EWrapperFactory {

 public:
  EWrapperFactory();
  ~EWrapperFactory();

  EWrapper* getImpl();

};


} // IBAPI
#endif IBAPI_EWRAPPER_FACTORY_FACTORY_H_
