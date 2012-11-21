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
set(test_service_order_manager_incs
  ${IBAPI_ROOT}
  ${IBAPI_ROOT}/Shared
  ${GEN_DIR}
  ${SRC_DIR}
  ${TEST_DIR}
)
set(test_service_order_manager_srcs
  ${TEST_DIR}/AllTests.cpp
  OrderManagerTest.cpp
)
if(TEST_WITH_MOCK)
  message(STATUS "Building with Mock")
  add_definitions('-DTEST_WITH_MOCK')
  set(test_service_order_manager_libs
    atp_order_manager
    IBAPIConnector
    ib_api_versioned_base
    )
else(TEST_WITH_MOCK)
  message(STATUS "Building with live connection")
  set(test_service_order_manager_libs
    atp_order_manager
    IBAPIConnector
    ib_api_versioned_base
    ib_api_versioned_handler # standard live protocol handler
    )
endif(TEST_WITH_MOCK)
cpp_gtest(test_service_order_manager)


# LogReader
set(test_service_log_reader_incs
  ${IBAPI_ROOT}
  ${IBAPI_ROOT}/Shared
  ${GEN_DIR}
  ${SRC_DIR}
  ${TEST_DIR}
)
set(test_service_log_reader_srcs
  ${TEST_DIR}/AllTests.cpp
  LogReaderTest.cpp
)
set(test_service_log_reader_libs
  atp_common
  atp_historian
  atp_zmq
  atp_log_reader
  boost_system
  boost_iostreams
  gflags
  glog
  protobuf-lite
  leveldb
  bz2
  z
  )
cpp_gtest(test_service_log_reader)


add_custom_target(all_service_tests)
add_dependencies(all_service_tests
  test_service_log_reader
  test_service_order_manager
)
