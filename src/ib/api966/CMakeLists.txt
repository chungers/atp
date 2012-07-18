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
# Version-specific base that includes message
# definitions, stream printers, and marshallers.
# This is separate from the actual handler for
# the api (EWrapper+EClient).  This is split
# so that tests can link against other libs that
# provide mocks of EClient.
set(ib_api_versioned_base_incs
  ${IBAPI_ROOT}
  ${IBAPI_ROOT}/Shared
)
set(ib_api_versioned_base_srcs
  ApiImpl.cpp
  MarketEventDispatcher.cpp
  OrderEventDispatcher.cpp
  marshall_contract.cpp
  marshall_order.cpp
  ostream.cpp
  ZmqMessage.cpp
)
set(ib_api_versioned_base_libs
  ib_dist
  gflags
  glog
  proto_components
  varz_components
)
cpp_library(ib_api_versioned_base)


##########################################
# Version-specific handler
# Split as separate here so that tests can
# customize and use mocks instead.
set(ib_api_versioned_handler_incs
  ${IBAPI_ROOT}
  ${IBAPI_ROOT}/Shared
)
set(ib_api_versioned_handler_srcs
  ApiProtocolHandler.cpp
)
set(ib_api_versioned_handler_libs
  ib_dist
  gflags
  glog
)
cpp_library(ib_api_versioned_handler)


# This library combines the two target libs above
# and is typically linked to include the version
# specific implementation for actual binaries.
FILE(GLOB VERSIONED_IMPL_FILES "*.cpp")
set(ib_api_versioned_incs
  ${IBAPI_ROOT}
  ${IBAPI_ROOT}/Shared
)
set(ib_api_versioned_srcs
  ${VERSIONED_IMPL_FILES}
)
set(ib_api_versioned_libs
  ib_api_versioned_handler
  ib_api_versioned_base
)
cpp_library(ib_api_versioned)


