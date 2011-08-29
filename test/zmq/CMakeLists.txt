#
# Tests for Zmq


# Complete Test Suite
set(ZmqTestSuite_incs
  ${IBAPI_ROOT}
  ${GEN_DIR}
  ${SRC_DIR}
  ${TEST_DIR}
)
set(ZmqTestSuite_srcs
  AllTests.cpp
)
set(ZmqTestSuite_libs
  IBAPIConnector
  ZmqComponents
)
cpp_gtest(ZmqTestSuite)

