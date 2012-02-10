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
  ApiMessageBase.cpp
  AsioEClientSocket.cpp
  SocketConnector.cpp
  SocketInitiator.cpp
  TickerMap.cpp
  ticker_id.cpp
  ${IMPL_FILES}
)
set(IBAPIConnector_libs
 ib_api   # version-specific: see IBAPI_IMPL_DIR
 varz_components
 zmq_components
 boost_system
 boost_thread
 gflags
 glog
 quickfix
 QuantLib
 zmq
)
# Client implementation:
cpp_library(IBAPIConnector)

##########################################
# Simple Log File Reader
set(LogReader_incs
  ${GEN_DIR}
  ${SRC_DIR}
)
set(LogReader_srcs
  logreader_main.cpp
)
set(LogReader_libs
  IBAPIConnector
  boost_system
  gflags
  glog
  common_proto
  historian_proto
  protobuf-lite
)
cpp_executable(LogReader)

# Add the versioned api subdirectory
add_subdirectory(${IBAPI_IMPL_DIR})

