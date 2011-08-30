#
# Tests for Zmq

set(API_IMPL ${SRC_DIR}/ib/${IBAPI_IMPL_DIR})

message(STATUS "Using IB API version ${IBAPI_VERSION} from ${IBAPI_ROOT}")
message(STATUS "Implementation directory = ${API_IMPL}")


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
  ZmqMessageTest.cpp
)
set(ZmqTestSuite_libs
  IBAPIConnector
  ZmqComponents
)
cpp_gtest(ZmqTestSuite)

