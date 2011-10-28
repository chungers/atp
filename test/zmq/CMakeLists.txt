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
  AllTests.cpp
  ReactorTest.cpp
)
set(ZmqTestSuite_libs
  IBAPIConnector
  ZmqComponents
)
cpp_gtest(ZmqTestSuite)

