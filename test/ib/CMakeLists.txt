#
# Tests for IB connector

message(STATUS "Tests: IB API version ${IBAPI_VERSION} from ${IBAPI_ROOT}")
include_directories(${IBAPI_ROOT}/Shared ${IBAPI_ROOT}/PosixSocketClient)

# Utils test
set(utils_test_incs
  ${GEN_DIR}
  ${SRC_DIR}
  ${TEST_DIR}
)
set(utils_test_srcs
  AllTests.cpp
  UtilsTest.cpp
)
set(utils_test_libs
  ib_adapter
  boost_thread
  gflags
  glog
)
cpp_gtest(utils_test)
