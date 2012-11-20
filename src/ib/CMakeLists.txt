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
  contract_symbol.cpp
)
set(api_base_libs
 atp_zmq
 boost_system
 boost_thread
 gflags
 glog
 zmq
)
cpp_library(api_base)

##########################################
# Connector to the IB API Gateway
# Note that this has dependency on the actual
# libraries that implement the hpp classes
# such as ApiProtocolHandler.hpp
set(IBAPIConnector_incs
  ${IBAPI_ROOT}
  ${IBAPI_ROOT}/Shared
  ${GEN_DIR}
  ${SRC_DIR}
  ${IBAPI_IMPL_DIR}
)
set(IBAPIConnector_srcs
  AsioEClientDriver.cpp
  SessionSetting.cpp
  SocketConnector.cpp
  SocketInitiator.cpp
  MarketEventDispatcher.cpp
  OrderEventDispatcher.cpp
)
set(IBAPIConnector_libs
 api_base
 atp_proto
 atp_varz
 atp_zmq
 boost_system
 boost_thread
 gflags
 glog
 zmq
)
cpp_library(IBAPIConnector)


# Add the versioned api subdirectory
add_subdirectory(${IBAPI_IMPL_DIR})

