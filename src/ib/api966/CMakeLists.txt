# Interactive Broker API adapter
#
message(STATUS "IB API v9.66 =================================================")
message(STATUS "-- IB API version ${IBAPI_VERSION} from ${IBAPI_ROOT}")

##########################################
# Build the IB api as a library

# Use standard string library instead of mvc foundation classes:
option(IB_USE_STD_STRING "Use standard c++ string (see Shared/IBString.h)." ON)
if (IB_USE_STD_STRING)
  add_definitions('-DIB_USE_STD_STRING')
endif (IB_USE_STD_STRING)

# Core classes in the IB api:
include_directories(${IBAPI_ROOT}/Shared)
ext_library(ib_dist
  ${IBAPI_ROOT}/PosixSocketClient/EClientSocketBase.cpp
)

##########################################
# Version-specific code
set(ib_api_incs
  ${IBAPI_ROOT}
  ${IBAPI_ROOT}/Shared
)
set(ib_api_srcs
  ApiImpl.cpp
  ApiProtocolHandler.cpp
  MarketEventDispatcher.cpp
  OrderEventDispatcher.cpp
  marshall_contract.cpp
  marshall_order.cpp
  ostream.cpp
  ZmqMessage.cpp
)
set(ib_api_libs
  ib_dist
  gflags
  glog
  proto_components
  varz_components
)
cpp_library(ib_api)



