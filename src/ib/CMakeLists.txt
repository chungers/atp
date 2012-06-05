# Interactive Broker API adapter
#
message(STATUS "IB API =======================================================")
message(STATUS "-- IB API version ${IBAPI_VERSION} from ${IBAPI_ROOT}")

add_definitions('-fPIC')

# IB Specific compile options
option(IB_USE_STD_STRING "Use standard c++ string (see Shared/IBString.h)." ON)
if (IB_USE_STD_STRING)
  add_definitions('-DIB_USE_STD_STRING')
endif (IB_USE_STD_STRING)

##########################################
# Base API lib to be included for version impls
set(api_base_incs
  ${IBAPI_ROOT}
  ${IBAPI_ROOT}/Shared
  ${GEN_DIR}
  ${SRC_DIR}
  ${IBAPI_IMPL_DIR}
)
set(api_base_srcs
  TickerMap.cpp
  ticker_id.cpp
)
set(api_base_libs
 zmq_components
 boost_system
 boost_thread
 gflags
 glog
 zmq
)
# Client implementation:
cpp_library(api_base)

##########################################
# Connector to the IB API Gateway
set(IBAPIConnector_incs
  ${IBAPI_ROOT}
  ${IBAPI_ROOT}/Shared
  ${GEN_DIR}
  ${SRC_DIR}
  ${IBAPI_IMPL_DIR}
)
FILE(GLOB IMPL_FILES "${IBAPI_IMPL_DIR}/*.cpp")
set(IBAPIConnector_srcs
  AsioEClientDriver.cpp
  SessionSetting.cpp
  SocketConnector.cpp
  SocketInitiator.cpp
  MarketEventDispatcher.cpp
  ${IMPL_FILES}
)
set(IBAPIConnector_libs
 api_base
 ib_api   # version-specific: see IBAPI_IMPL_DIR
 proto_components
 varz_components
 zmq_components
 boost_system
 boost_thread
 gflags
 glog
 zmq
)
# Client implementation:
cpp_library(IBAPIConnector)



# Add the versioned api subdirectory
add_subdirectory(${IBAPI_IMPL_DIR})

