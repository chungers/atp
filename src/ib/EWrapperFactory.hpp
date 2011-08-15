#ifndef IBAPI_EWRAPPER_FACTORY_H_
#define IBAPI_EWRAPPER_FACTORY_H_

#include <Shared/EWrapper.h>

namespace IBAPI {


class EWrapperFactory {

 public:
  ~EWrapperFactory() {}

  virtual EWrapper* getImpl() = 0;


  static EWrapperFactory* getInstance();
};



} // IBAPI
#endif IBAPI_EWRAPPER_FACTORY_H_
