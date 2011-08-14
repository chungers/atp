# Interactive Broker API adapter
#

##########################################
# Build the IB api as a library

# Use standard string library instead of mvc foundation classes:
option(IB_USE_STD_STRING "Use standard c++ string (see Shared/IBString.h)." ON)
if (IB_USE_STD_STRING)
  add_definitions('-DIB_USE_STD_STRING')
endif (IB_USE_STD_STRING)

# Core classes in the IB api:
message(STATUS "Using IB API version ${IBAPI_VERSION} from ${IBAPI_ROOT}")
include_directories(${IBAPI_ROOT}/Shared ${IBAPI_ROOT}/PosixSocketClient)
ext_library(ib_api
  ${IBAPI_ROOT}/PosixSocketClient/EClientSocketBase.cpp
  ${IBAPI_ROOT}/PosixSocketClient/EPosixClientSocket.cpp
)

##########################################
# Logging adapter + utilities for ticker id conversion
set(ib_adapter_incs
  ${IBAPI_ROOT}
  ${IBAPI_ROOT}/PosixSocketClient
  ${IBAPI_ROOT}/Shared
  ${GEN_DIR}
  ${SRC_DIR}
  ${IBAPI_IMPL_DIR}
)
set(ib_adapter_srcs
  AsioEClientSocket.cpp
  SocketConnector.cpp
  SocketInitiator.cpp
  ticker_id.cpp
  ${IBAPI_IMPL_DIR}/ApiImpl.cpp
  ${IBAPI_IMPL_DIR}/Messages.cpp
)
set(ib_adapter_libs
  ib_api
)
# Client implementation:
cpp_library(ib_adapter)

