#
# Tests for Services

message(STATUS "TESTS: Services ==============================================")
message(STATUS "-- IBAPI_IMPL_DIR = ${SRC_DIR}/ib/${IBAPI_IMPL_DIR}")


option(TEST_WITH_MOCK "Use mock when testing order manager." ON)

option(IB_USE_STD_STRING "Use standard c++ string (see Shared/IBString.h)." ON)
if (IB_USE_STD_STRING)
  add_definitions('-DIB_USE_STD_STRING')
endif (IB_USE_STD_STRING)

##########################################
# customize and use mocks instead.
set(api_protocol_handler_incs
  ${IBAPI_ROOT}
  ${IBAPI_ROOT}/Shared
  ${SRC_DIR}/ib/{IBAPI_IMPL_DIR}
)
set(api_protocol_handler_srcs
  ApiProtocolHandler.cpp
)
set(api_protocol_handler_libs
  ib_dist
  gflags
  glog
)
cpp_library(api_protocol_handler)

# OrderManager
set(OrderManagerTest_incs
  ${IBAPI_ROOT}
  ${IBAPI_ROOT}/Shared
  ${GEN_DIR}
  ${SRC_DIR}
  ${TEST_DIR}
)
set(OrderManagerTest_srcs
  ${TEST_DIR}/AllTests.cpp
  OrderManagerTest.cpp
)
if(TEST_WITH_MOCK)
  message(STATUS "Building with Mock")
  add_definitions('-DTEST_WITH_MOCK')
  set(OrderManagerTest_libs
    order_manager
    IBAPIConnector
    ib_api_versioned_base
    )
else(TEST_WITH_MOCK)
  message(STATUS "Building with live connection")
  set(OrderManagerTest_libs
    order_manager
    IBAPIConnector
    ib_api_versioned_base
    ib_api_versioned_handler # standard live protocol handler
    )
endif(TEST_WITH_MOCK)
cpp_gtest(OrderManagerTest)


# LogReader
set(LogReaderTest_incs
  ${IBAPI_ROOT}
  ${IBAPI_ROOT}/Shared
  ${GEN_DIR}
  ${SRC_DIR}
  ${TEST_DIR}
)
set(LogReaderTest_srcs
  ${TEST_DIR}/AllTests.cpp
  LogReaderTest.cpp
)
set(LogReaderTest_libs
  historian_components
  zmq_components
  log_reader
  boost_system
  boost_iostreams
  gflags
  glog
  protobuf-lite
  leveldb
  bz2
  z
  )
cpp_gtest(LogReaderTest)


add_custom_target(all_service_tests)
add_dependencies(all_service_tests
  LogReaderTest
  OrderManagerTest
)
