#
# Tests for Zmq

set(API_IMPL ${SRC_DIR}/ib/${IBAPI_IMPL_DIR})

message(STATUS "TESTS: ZeroMQ ================================================")

# Complete Test Suite
set(ZmqTestSuite_incs
  ${IBAPI_ROOT}
  ${GEN_DIR}
  ${SRC_DIR}
  ${TEST_DIR}
  ${API_IMPL}
)
set(ZmqTestSuite_srcs
  ${TEST_DIR}/AllTests.cpp
  ReactorTest.cpp
)
set(ZmqTestSuite_libs
  IBAPIConnector
  zmq_components
)
cpp_gtest(ZmqTestSuite)

add_custom_target(all_zmq_tests)
add_dependencies(all_zmq_tests
  ZmqTestSuite
)
